#if defined(macintosh)
#include <types.h>
#else
#include <sys/types.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <cmath>
#include <sstream>
#include <iomanip>
#include <cstring>
#include "merc.h"
#include "tables.h"
#include "lookup.h"
#include "magic.h"
#include "spells_fire.h"
#include "spells_void.h"
#include "interp.h"
#include "songs.h"
#include "EchoAffect.h"

/* External declarations */
DECLARE_DO_FUN(do_mpfocus);
DECLARE_DO_FUN(do_autoyell);
DECLARE_DO_FUN(do_look);

DECLARE_PRO_FUN(prog_focus);

extern	bool	global_bool_ranged_attack;
extern bool global_bool_check_avoid;

PyreInfo * getPyreInfoEffect(CHAR_DATA * ch, PyreInfo::Effect effect)
{
    // Check for the existence of the affect
    AFFECT_DATA * pyre(get_affect(ch, gsn_bloodpyre));
    if (pyre == NULL || pyre->point == NULL)
        return NULL;
    
    // Check that this effect is in existence
    PyreInfo * pyreInfo(static_cast<PyreInfo*>(pyre->point));
    if (pyreInfo->effect() == effect)
        return pyreInfo;

    return NULL;
}

std::vector<std::pair<CHAR_DATA*, PyreInfo*> > getPyreInfoEffectsArea(CHAR_DATA * ch, PyreInfo::Effect effect)
{
    std::vector<std::pair<CHAR_DATA *, PyreInfo*> > result;
    if (ch->in_room == NULL)
        return result;

    // Only players can have pyres
    for (DESCRIPTOR_DATA * d(descriptor_list); d != NULL; d = d->next)
    {
        if (d->connected != CON_PLAYING || d->character->in_room == NULL || d->character->in_room->area != ch->in_room->area)
            continue;

        PyreInfo * pyreInfo(getPyreInfoEffect(d->character, effect));
        if (pyreInfo == NULL)
            continue;

        result.push_back(std::make_pair(d->character, pyreInfo));
    }

    return result;
}

CHAR_DATA * findCharForPyre(OBJ_DATA * pyre)
{
    for (CHAR_DATA * ch(char_list); ch != NULL; ch = ch->next)
    {
        // Check for pyre affect
        AFFECT_DATA * pyreAffect(get_affect(ch, gsn_bloodpyre));
        if (pyreAffect == NULL || pyreAffect->point == NULL)
            continue;

        // Get the pyre info
        PyreInfo * pyreInfo(static_cast<PyreInfo*>(pyreAffect->point));
        if (pyre == pyreInfo->pyre())
            return ch;
    }

    return NULL;
}

void checkAutoCauterize(CHAR_DATA * ch, int sn)
{
    // Flameheart will cauterize
    if (number_percent() < get_skill(ch, gsn_flameheart) / 2)
    {
        send_to_char("You feel your open wounds cauterize shut from the heat of your own blood!\n", ch);
        if (ch->in_room != NULL)
            act("As $n's blood steams out of $m, $s wounds cauterize shut!", ch, NULL, NULL, TO_ROOM);

        affect_strip(ch, sn);
        return;
    }

    // Check embrace of the deeps
    check_embraceofthedeeps_cauterize(*ch, sn);
}

void checkApplySmoke(int level, CHAR_DATA * victim)
{
    // Blind people or immune can't be affected by smoke
    if (IS_AFFECTED(victim, AFF_BLIND) || IS_SET(victim->imm_flags, IMM_BLIND))
        return;

    // Ghosts and anything wizi or voidwalking are immune to smoke
    if (IS_AFFECTED(victim, AFF_WIZI) || IS_PAFFECTED(victim, AFF_VOIDWALK) || IS_OAFFECTED(victim, AFF_GHOST) || is_affected(victim, gsn_astralprojection))
        return;

    // Firekin gives up to a 50% chance of avoiding the smoke effects
    if (number_percent() < get_skill(victim, gsn_firekin) / 2)
    {
        send_to_char("Smoke swirls about you, but you ignore it with practiced ease.\n", victim);
        check_improve(victim, NULL, gsn_firekin, TRUE, 4);
        return;
    }

    // Mistral ward gives a 50% chance of avoiding the smoke effects
    if (number_bits(1) == 0 && is_affected(victim, gsn_mistralward))
    {
        send_to_char("Smoke swirls about you, but your mistral ward keeps you from harm.\n", victim);
        return;
    }

    // Check the normal save
    if (saves_spell(level, NULL, victim, DAM_FIRE))
        return;

    // Nothing left to save you
    act("$n is blinded by the smoke!", victim, NULL, NULL, TO_ROOM);
    act("You are blinded by the smoke!", victim, NULL, NULL, TO_CHAR);

    AFFECT_DATA af = {0};
    af.where     = TO_AFFECTS;
    af.type      = gsn_smoke;
    af.level     = level;
    af.location  = APPLY_HITROLL;
    af.modifier  = -4;
    af.duration  = 1;
    af.bitvector = AFF_BLIND;
    affect_to_char(victim, &af);
}

void fillRoomWithSmoke(CHAR_DATA * ch, ROOM_INDEX_DATA * room, int level, int duration)
{
    // Sanity-check
    if (room == NULL)
        return;

    // Put the affect in the room
    AFFECT_DATA af = {0};
    af.where     = TO_ROOM_AFF;
    af.type      = gsn_smoke;
    af.level     = level;
    af.duration  = duration;
    affect_to_room(room, &af);

    // Now check everyone in the room against the smoke
    for (CHAR_DATA * victim = room->people; victim != NULL; victim = victim->next_in_room)
    {
        if (ch == NULL || !is_safe_spell(ch, victim, true))
            checkApplySmoke(level, victim);
    }
}

void sourcelessDamage(CHAR_DATA * victim, const char * message, int amount, int sn, int dam_type)
{
    static std::vector<DamageInfo> damage(1);
    damage[0].type = dam_type;
    damage[0].amount = amount;
    sourcelessDamage(victim, message, sn, damage);
}

void sourcelessDamage(CHAR_DATA * victim, const char * message, int sn, std::vector<DamageInfo> & damage)
{
    global_bool_check_avoid = FALSE;
    damage_new(NULL, victim, damage, sn, true, NULL, message);
    global_bool_check_avoid = TRUE;
}

static void damageRoomInAbsentia(CHAR_DATA * ch, ROOM_INDEX_DATA * room, const char * message, int amount, int sn, int dam_type)
{
    global_bool_check_avoid = FALSE;

    // Iterate the people in the room
    CHAR_DATA * victim_next;
    for (CHAR_DATA * victim = room->people; victim != NULL; victim = victim_next)
    {
        // Save off the next victim in case the link invalidates due to the damage
        victim_next = victim->next_in_room;

        // Only hurt valid targets
        if (!is_safe_spell(ch, victim, true))
            damage_new(NULL, victim, amount, sn, dam_type, true, NULL, message);
    }

    global_bool_check_avoid = TRUE;
}

static void meltIcyPrisonMessage(ROOM_INDEX_DATA * room)
{
    if (room->people != NULL)
    {
        act("The icy prison melts away from the power of the fire magic.", room->people, NULL, NULL, TO_CHAR);
        act("The icy prison melts away from the power of the fire magic.", room->people, NULL, NULL, TO_ROOM);
    }
}

static void checkMeltIcyPrisons(ROOM_INDEX_DATA * room)
{
    // Sanity-check
    if (room == NULL)
        return;

    // Try to melt an icy prison that is this room
    AFFECT_DATA * paf = get_room_affect(room, gsn_icyprison);
    if (paf != NULL)
    {
        meltIcyPrisonMessage(room);
        room_affect_strip(room, gsn_icyprison);
        destroy_icyprison(room, get_room_index(paf->modifier));
    }

    // Try to melt an icy prison in this room
    OBJ_DATA * obj_next;
    for (OBJ_DATA * obj = room->contents; obj != NULL; obj = obj_next)
    {
        obj_next = obj->next_content;
        if (obj->pIndexData->vnum == OBJ_VNUM_ICY_PRISON)
        {
            paf = affect_find(obj->affected, gsn_icyprison);
            if (paf->point != NULL)
            {
                ROOM_INDEX_DATA * pafRoom = static_cast<ROOM_INDEX_DATA*>(paf->point);
                meltIcyPrisonMessage(pafRoom);
                room_affect_strip(pafRoom, gsn_icyprison);
                destroy_icyprison(pafRoom, room);
            }
        }
    }
}

OBJ_DATA * lookupPyre(ROOM_INDEX_DATA * room)
{
    for (OBJ_DATA * obj(room->contents); obj != NULL; obj = obj->next_content)
    {
        if (obj->pIndexData->vnum == OBJ_VNUM_PYRE)
            return obj;
    }

    return NULL;
}

void stopFiredancing(CHAR_DATA * ch, AFFECT_DATA * paf, int skill)
{
    // Cooldown starts at 15, but is reduced by 1 for every 10% above 50%; at 100%, this mean a cooldown of 10
    paf->duration = UMAX(1, 20 - (UMAX(0, (skill - 50)) / 10));
    paf->modifier = -1 * UMIN(paf->modifier, 100);
    paf->location = APPLY_MANA;
    ch->max_mana += paf->modifier;
    ch->mana = UMIN(ch->mana, ch->max_mana);
    act("You shut yor mind to the preternatural melody, and realize you are covered in perspiration.", ch, NULL, NULL, TO_CHAR);
    act("$n's face loses its color even as $e breaks out into a heavy sweat.", ch, NULL, NULL, TO_ROOM);
}

void do_incinerate(CHAR_DATA * ch, char * argument)
{
    // Lookup the affect
    AFFECT_DATA * paf(get_affect(ch, gsn_bloodpyre));
    if (paf == NULL || paf->point == NULL)
    {
        send_to_char("You have no link to the Inferno in existence.\n", ch);
        return;
    }

    // Sanity check
    if (ch->in_room == NULL)
    {
        bug("do_incinerate called in NULL room", 0);
        send_to_char("You aren't anywhere.\n", ch);
        return;
    }

    // Lookup the pyre
    PyreInfo * pyreInfo(static_cast<PyreInfo*>(paf->point));
    OBJ_DATA * pyre(lookupPyre(ch->in_room));
    if (pyre != pyreInfo->pyre())
    {
        send_to_char("You are not close enough to your pyre to incinerate anything.\n", ch);
        return;
    }

    // Check for mana
    if (ch->mana < 20)
    {
        send_to_char("As tired as you are, you would not be able to summon the will to do anything more than simply destroy it.\n", ch);
        return;
    }

    // Get the target object name
    char arg[MAX_INPUT_LENGTH];
    argument = one_argument(argument, arg);
    if (arg[0] == '\0')
    {
        send_to_char("What did you want to incinerate?\n", ch);
        return;
    }

    // Get the target object
    OBJ_DATA * obj(get_obj_carry(ch, arg, ch));
    if (obj == NULL)
    {
        // Not a direct object, but check for coins
        char arg2[MAX_INPUT_LENGTH];
        one_argument(argument, arg2);
        int amount = atoi(arg);
        int coinType = coin_lookup(arg2);
        if (amount <= 0 || coinType < 0)
        {
            std::ostringstream mess;
            mess << "You don't see any " << arg << " here.\n";
            send_to_char(mess.str().c_str(), ch);
            return;
        }

        // Coins chosen; make sure the character has enough
        if (ch->coins[coinType] < amount)
        {
            std::ostringstream mess;
            mess << "You aren't carrying that much " << coin_table[coinType].name << ".\n";
            send_to_char(mess.str().c_str(), ch);
            return;
        }

        // Coins check out, so make the coin object
        coins_from_char(ch, amount, coinType);
        obj = create_money(amount, coinType);
    }
    else if (IS_SET(obj->extra_flags[0], ITEM_NODESTROY))
    {
        act("Not even the fires of your pyre could incinerate $p.", ch, obj, NULL, TO_CHAR);
        return;
    }

    // Incinerate the object
    expend_mana(ch, 20);
    WAIT_STATE(ch, UMAX(ch->wait, (3 * PULSE_VIOLENCE) / 2));
    if (pyreInfo->incinerateObject(ch, obj))
        extract_obj(obj);
}

void do_firedancer(CHAR_DATA * ch, char *)
{
    // Make sure they have the skill
    int skill = get_skill(ch, gsn_firedancer);
    if (skill <= 0)
    {
        send_to_char("You have not been initiated into the mysteries of the firedancer.\n", ch);
        return;
    }

    // Check whether the affect is already present
    AFFECT_DATA * paf(get_affect(ch, gsn_firedancer));
    if (paf == NULL)
    {
        // Check for mana
        if (ch->mana < skill_table[gsn_firedancer].min_mana)
        {
            send_to_char("You are too weary to open your mind to the flames.\n", ch);
            return;
        }

        // Affect not present, so turn it on
        AFFECT_DATA af = {0};
        af.where     = TO_AFFECTS;
        af.type      = gsn_firedancer;
        af.level     = ch->level;
        af.duration  = -1;
        af.modifier  = 10 + dice(2, 10);
        affect_to_char(ch, &af);

        act("Your senses fill with a feverish pounding as you open your mind to the rhythmic flicker of the flames.", ch, NULL, NULL, TO_CHAR);
        act("$n's face abruptly flushes, and $e begins moving more rhythmically.", ch, NULL, NULL, TO_ROOM);
        
        WAIT_STATE(ch, UMAX(ch->wait, skill_table[gsn_firedancer].beats));
        expend_mana(ch, skill_table[gsn_firedancer].min_mana);
        return;
    }

    // Affect already exists; check whether it is on or just cooling down
    if (paf->duration < 0)
    {
        // It is currently on; turn it off
        stopFiredancing(ch, paf, skill);
        return;
    }

    // Affect is cooling down
    send_to_char("You are not yet ready to open your mind to the flames again.\n", ch);
}

static std::vector<std::pair<OBJ_DATA*, AFFECT_DATA*> > lookupSealObjects(CHAR_DATA * ch)
{
    std::vector<std::pair<OBJ_DATA*, AFFECT_DATA*> > result;

    // Iterate the world's objects
    for (OBJ_DATA * obj(object_list); obj != NULL; obj = obj->next)
    {
        // Iterate the object's affects
        for (AFFECT_DATA * paf(obj->affected); paf != NULL; paf = paf->next)
        {
            // Look for a gsn and target match
            if (paf->type == gsn_sealofthegoldenflames && paf->point == ch)
            {
                result.push_back(std::make_pair(obj, paf));
                break;
            }
        }
    }
    return result;
}

static const int SealType_Explode = 0;
static const int SealType_Smoke = 1;

static void triggerSealObject(CHAR_DATA * ch, OBJ_DATA * obj, AFFECT_DATA * paf)
{
    // Get the room for the object
    ROOM_INDEX_DATA * room(get_room_for_obj(*obj));
    if (room == NULL)
    {
        // Nowhere to explode the object
        object_affect_strip(obj, gsn_sealofthegoldenflames);
        return;
    }

    // Check for underwater
    if (room->sector_type == SECT_UNDERWATER)
    {
        act("$p suddenly flares brightly with momentary heat, but is quickly cooled by the water.", room->people, obj, NULL, TO_CHAR);
        act("$p suddenly flares brightly with momentary heat, but is quickly cooled by the water.", room->people, obj, NULL, TO_ROOM);
    }

    // Check for the type of seal
    switch (paf->modifier)
    {
        case SealType_Explode:
        {
            // Explode the seal
            unsigned int damage = 10 + dice(4, paf->level);
            if (IS_SET(obj->extra_flags[0], ITEM_NODESTROY))
            {
                // The object cannot be destroyed
                if (room->people != NULL)
                {
                    act("$p suddenly flares brightly, and a ball of flame bursts from it!", room->people, obj, NULL, TO_CHAR);
                    act("$p suddenly flares brightly, and a ball of flame bursts from it!", room->people, obj, NULL, TO_ROOM);
                    object_affect_strip(obj, gsn_sealofthegoldenflames);
                }
            }
            else
            {
                // The object can be destroyed
                if (room->people != NULL)
                {
                    act("$p suddenly flares brightly, then explodes in a ball of flame and shrapnel!", room->people, obj, NULL, TO_CHAR);
                    act("$p suddenly flares brightly, then explodes in a ball of flame and shrapnel!", room->people, obj, NULL, TO_ROOM);
                }

                // Extra damage for a destroyed object, due to the shrapnel
                damage += dice(1, paf->level);
                extract_obj(obj);
            }
            
            // Check for melting icy prisons after exploding
            damageRoomInAbsentia(ch, room, "the fiery explosion", damage, gsn_sealofthegoldenflames, DAM_FIRE);
            checkMeltIcyPrisons(room);
            break;
        }

        case SealType_Smoke: 
        {
            // Cause smoke in the room
            if (room->people != NULL)
            {
                act("$p suddenly sparks and fogs over, then smoke begins to issue forth from it!", room->people, obj, NULL, TO_CHAR);
                act("$p suddenly sparks and fogs over, then smoke begins to issue forth from it!", room->people, obj, NULL, TO_ROOM);
            }
            
            fillRoomWithSmoke(ch, room, UMAX(1, paf->level - 3), 3);
            object_affect_strip(obj, gsn_sealofthegoldenflames);
            break;
        }

        default: 
        {
            // Log this bug
            std::ostringstream mess;
            mess << "Invalid seal type: " << paf->modifier << "[Char: '" << ch->name << "']";
            bug(mess.str().c_str(), 0);
            object_affect_strip(obj, gsn_sealofthegoldenflames);
            break;
        }
    }
}

bool spell_sealofthegoldenflames(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    // Cannot cast this as a ghost
    if (IS_OAFFECTED(ch, AFF_GHOST))
    {
        send_to_char("The flames are too strong for your ghostly form to command.\n", ch);
        return false;
    }

    // Check whether this is being cast with or without an argument
    std::vector<std::pair<OBJ_DATA *, AFFECT_DATA *> > sealObjects(lookupSealObjects(ch));
    if (target_name[0] == '\0')
    {
        // Make sure they actually have a seal in place
        if (sealObjects.size() == 0)
        {
            send_to_char("You have no active Seals of the Golden Flames in place.\n", ch);
            return false;
        }

        // Trigger the seal(s)
        for (size_t i(0); i < sealObjects.size(); ++i)
            triggerSealObject(ch, sealObjects[i].first, sealObjects[i].second);

        return true;
    }

    // Make sure the caster does not have the cooldown on them
    if (is_affected(ch, sn))
    {
        send_to_char("You are not yet ready to place another Seal of the Golden Flames.\n", ch);
        return false;
    }

    // Verify that the caster can place more seals
    // All users get one sealed object, hero-level spells and those who have mastered the skill get a bonus one
    // Range: [1 - 3]; if used by someone without the skill (eg, via wand), then range is [1 - 2]
    if (sealObjects.size() >= static_cast<unsigned int>(1 + (level >= 51 ? 1 : 0) + ((get_skill(ch, sn) >= 100) ? 1 : 0)))
    {
        send_to_char("You lack the power to maintain any additional seals.\n", ch);
        return false;
    }

    // Read off the target
    char arg[MAX_INPUT_LENGTH];
    const char * argument = one_argument(target_name, arg);
    OBJ_DATA * obj = get_obj_here(ch, arg);
    if (obj == NULL)
    {
        // Invalid object targeted
        std::ostringstream mess;
        mess << "You do not see any " << arg << " around here.\n";
        send_to_char(mess.str().c_str(), ch);
        return false;
    }

    // Make sure it only applies to items of type light
    if (obj->item_type != ITEM_LIGHT)
    {
        act("$p cannot be marked with a Seal.", ch, obj, NULL, TO_CHAR);
        return false;
    }

    // Read off the type of seal, either "explode" or "smoke"
    AFFECT_DATA af = {0};
    argument = one_argument(const_cast<char*>(argument), arg);
    if (strcmp(arg, "explode") == 0) af.modifier = SealType_Explode;
    else if (strcmp(arg, "smoke") == 0) af.modifier = SealType_Smoke;
    else
    {
        // Invalid seal type
        std::ostringstream mess;
        mess << "With which type of Seal did you wish to imbue " << obj->short_descr << "?\n";
        send_to_char(mess.str().c_str(), ch);
        return false;
    }

    // Make sure the object does not already have a seal
    AFFECT_DATA * paf = get_obj_affect(obj, sn);
    if (paf != NULL)
    {
        // Found an existing seal; trigger it
        act("You start to imbue $p with a Seal of the Golden Flames, but it already has one.", ch, obj, NULL, TO_CHAR);
        act("Before you can stop it, the combined forces of combustion overwhelm $p!", ch, obj, NULL, TO_CHAR);
        triggerSealObject(ch, obj, paf);
        return true;
    }

    // Time to set up the actual affect on the object
    // Point the affect to the caster
    af.point        = ch;
    af.where        = TO_OBJECT;
    af.type         = sn;
    af.level        = level;
    af.duration     = level * 2;
    affect_to_obj(obj, &af);

    // Add a small cooldown to the caster
    memset(&af, 0, sizeof(AFFECT_DATA));
    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = 4;
    affect_to_char(ch, &af);

    // Inform the caster/room
    act("You trace the Seal of the Golden Flames over $p, and feel its energies surge with barely-constrained power.", ch, obj, NULL, TO_CHAR);
    act("$n traces $s finger over $p, causing it to sparkle brightly for a moment.", ch, obj, NULL, TO_ROOM);
    return true;
}

bool spell_temper(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    // Get the object
    char arg[MAX_INPUT_LENGTH];
    char * argument = one_argument(target_name, arg);
    OBJ_DATA * obj(get_obj_carry(ch, arg, ch));
    if (obj == NULL)
    {
        send_to_char("You don't see that here.\n", ch);
        return false;
    }

    // No underwater tempering
    if (ch->in_room && ch->in_room->sector_type == SECT_UNDERWATER)
    {
        send_to_char("You cannot summon sufficient flame underwater.\n", ch);
        return false;
    }

    // Make sure the object is not already tempered
    if (IS_SET(obj->extra_flags[0], ITEM_NODESTROY) || obj_is_affected(obj, sn))
    {
        act("$p is too strong to respond to your tempering.", ch, obj, NULL, TO_CHAR);
        return false;
    }

    // Make sure the object is ferric
    if (!material_table[obj->material].ferric)
    {
        send_to_char("Only ferric materials may be tempered.\n", ch);
        return false;
    }

    // Check for risk level
    one_argument(argument, arg);
    int modifier;
    const char * adverb;
    if (strcmp(arg, "low") == 0) 
    {
        modifier = 1;
        adverb = "skillfully";
    }
    else if (strcmp(arg, "high") == 0) 
    {
        modifier = 2;
        adverb = "masterfully";
    }
    else
    {
        send_to_char("You must specify the temperature to use when tempering, either 'high' or 'low'.\n", ch);
        return false;
    }

    // Tempering started
    act("You chant softly, and $p begins to warm. In moments, it is glowing white-hot.", ch, obj, NULL, TO_CHAR);
    act("$n chants softly, and $p begins to warm. In moments, it is glowing white-hot.", ch, obj, NULL, TO_ROOM);
    
    // Check for destroying the item
    int skill = get_skill(ch, sn);
    int destroyChance = (20 - UMAX(0, (skill - 60) / 5)) * modifier * modifier;
    if (number_percent() < URANGE(5, destroyChance, 95))
    {
        act("$p grows too hot, and starts to melt! Before you can react, there is nothing left of it but a puddle of rapidly-cooling metal.", ch, obj, NULL, TO_CHAR);
        act("$p grows too hot, and starts to melt! In short order there is nothing left of it but a puddle of rapidly-cooling metal.", ch, obj, NULL, TO_ROOM);
        extract_obj(obj);
        return true;
    }

    // Temper the object according to type
    AFFECT_DATA af = {0};
    af.where        = TO_OBJECT;
    af.type         = sn;
    af.level        = level;
    af.duration     = -1;
    af.bitvector    = ITEM_NODESTROY;

    if (obj->item_type == ITEM_WEAPON)
    {
        // Weapons get 1 hitroll, plus 1 more for hero-level spells
        af.location     = APPLY_HITROLL;
        af.modifier     = modifier + (level >= 51 ? 1 : 0);
        affect_to_obj(obj, &af);

        // Bonus for skill
        if (number_percent() < ((skill * modifier) / 4))
        {
            af.bitvector = 0;
            af.location  = APPLY_DAMROLL;
            af.modifier  = 1;
            affect_to_obj(obj, &af);
        }
    }
    else
    {
        // Armor/other gets an AC for every 7 levels (or 2 per 7 for riskier), plus a bonus 2 if mastered
        af.location  = APPLY_AC;
        af.modifier  = -1 * ((UMAX(1, ((level - 2) / 7)) * modifier) + (skill >= 100 ? 2 : 0));
        affect_to_obj(obj, &af);
    }

    // Inform of success
    std::ostringstream mess;
    mess << "You " << adverb << " heat $p to just below its critical point, then, satisfied in your work, allow it to cool.";
    act(mess.str().c_str(), ch, obj, NULL, TO_CHAR);
    act("$n's chanting slowly comes to a stop, and $p begins to cool.", ch, obj, NULL, TO_ROOM);
    return true;
}

static bool canSeeGroundFire(CHAR_DATA * ch, OBJ_DATA * obj)
{
    // Only count valid, visible, fire objects on the ground
    return (obj->valid && can_see_obj(ch, obj) && IS_OBJ_STAT_EXTRA(obj, ITEM_FIRE) && obj->in_room != NULL && obj->carried_by == NULL);
}

std::vector<OBJ_DATA *> lookupGroundFires(CHAR_DATA * ch, OBJ_DATA * ignore, bool allowCursed)
{
    std::vector<OBJ_DATA *> result;
    for (OBJ_DATA * obj(object_list); obj != NULL; obj = obj->next )
    {
        // Ignore objects that the looker cannot see, that match the ignore object, that aren't fire, and that aren't sitting on the ground
        if (!canSeeGroundFire(ch, obj) || obj == ignore || obj->in_room->area->area_flags & AREA_UNCOMPLETE)
            continue;

        // If so requested, also ignore objects/looker in cursed locations
        if (!allowCursed && 
           (IS_SET(obj->in_room->room_flags, ROOM_NOMAGIC)
        ||  IS_SET(obj->in_room->room_flags, ROOM_SAFE)
        ||  IS_SET(obj->in_room->room_flags, ROOM_PRIVATE)
        ||  IS_SET(obj->in_room->room_flags, ROOM_NO_RECALL)
        ||  IS_SET(obj->in_room->room_flags, ROOM_IMP_ONLY)
        ||  IS_SET(obj->in_room->room_flags, ROOM_GODS_ONLY)
        ||  (IS_SET(obj->in_room->room_flags, ROOM_HEROES_ONLY) && ch->level < 51)
        ||  (IS_SET(obj->in_room->room_flags, ROOM_NEWBIES_ONLY) && ch->level > 10)
        ||  IS_SET(obj->in_room->room_flags, ROOM_NO_RECALL)
        ||  IS_SET(ch->in_room->room_flags,  ROOM_NO_RECALL)))
            continue;

        // Fire checks out
        result.push_back(obj);
    }
    return result;
}

OBJ_DATA * lookupGroundFireHere(CHAR_DATA * ch)
{
    // Verify that the character is in a room
    if (ch->in_room == NULL)
        return NULL;

    for (OBJ_DATA * obj = ch->in_room->contents; obj != NULL; obj = obj->next_content)
    {
        // Stop on the first visible ground fire
        if (canSeeGroundFire(ch, obj))
            return obj;
    }

    return NULL;
}

bool spell_flamesight(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    // Make sure there is a fire here
    OBJ_DATA * obj(lookupGroundFireHere(ch));
    if (obj == NULL)
    {
        send_to_char("There are no suitable flames here.\n", ch);
        return false;
    }

    // Get a list of all eligible fires
    std::vector<OBJ_DATA*> fires(lookupGroundFires(ch, obj, false));
    char arg[MAX_INPUT_LENGTH];
    one_argument(target_name, arg);

    // Check whether a fire was chosen
    if (arg[0] == '\0')
    {
        // No fire chosen
        act("For a long moment, $n stares deeply into $p, $s gaze intense and searching.", ch, obj, NULL, TO_ROOM);
       
        // Check for any fire 
        if (fires.size() == 0)
        {
            send_to_char("You stare intently into the flames, but cannot discern any other links to the Inferno.\n", ch);
            return true;
        }

        // Send a list of fires
        send_to_char("You stare intently into the fire, and slowly begin to discern links to the Inferno.\n", ch);
        send_to_char("Through the flames you see...\n", ch);

        // Determine how many places to hold for the number
        size_t placeCount = 0;
        for (size_t count(fires.size()); count > 0; count /= 10)
            ++placeCount;

        std::ostringstream mess;
        mess << std::setfill(' ');
        for (size_t i(0); i < fires.size(); ++i)
        {
            mess.str("");
            mess << '[' << std::setw(placeCount) << (i + 1) << "] " << fires[i]->short_descr << " is in " << fires[i]->in_room->name << " (" << fires[i]->in_room->area->name << ")\n";
            send_to_char(mess.str().c_str(), ch);
        }

        return true;
    }

    // A fire was chosen
    std::istringstream in(arg);
    size_t fireIndex;
    if (!(in >> fireIndex))
    {
        send_to_char("If you wish to see through the eyes of a flame, you must specify which one.\n", ch);
        return false;
    }

    // Map the 1-based index to a proper 0-based index
    --fireIndex;
    if (fireIndex >= fires.size())
    {
        send_to_char("You sense no such flame upon this plane.\n", ch);
        return false;
    }

    // Fire selected and verified, perform the spell
    act("For a long moment, $n stares deeply into $p, $s gaze intense and searching.", ch, obj, NULL, TO_ROOM);
    send_to_char("You stare through the heart of the Inferno itself, and a vision unfolds before you.\n", ch);
    send_to_char("Through the flames you see...\n", ch);

    // Pull the character into the target room
    ROOM_INDEX_DATA * originalRoom(ch->in_room);
    char_from_room(ch);
    char_to_room(ch, fires[fireIndex]->in_room);

    // Do a look and a where
    do_look(ch, "auto");
    send_to_char("\n", ch);
    do_where(ch, "");

    // Return the character to the original room
    char_from_room(ch);
    char_to_room(ch, originalRoom);
    return true;
}

void do_bloodpyre(CHAR_DATA * ch, char * argument)
{
    CHAR_DATA * victim(ch);
    if (IS_IMMORTAL(ch))
    {
        // Imms can look at other people's bloodpyre info
        if (argument[0] != '\0')
        {
            victim = get_char_world(ch, argument);
            if (victim == NULL)
            {
                send_to_char("You don't see them in the world.\n", ch);
                return;
            }
        }
    }
    else if (get_skill(ch, gsn_bloodpyre) <= 0)
    {
        // Make sure they have the skill
        send_to_char("Huh?\n", ch);
        return;
    }

    // Show the result
    if (victim == ch) act("You are learned in the ways of these bloodpyres:", ch, NULL, NULL, TO_CHAR);
    else act("$N is learned in the ways of these bloodpyres:", ch, NULL, victim, TO_CHAR);
    send_to_char(PyreInfo::listKnownEffects(victim).c_str(), ch);
}

bool spell_bloodpyre(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    // Verify room
    ROOM_INDEX_DATA * room(ch->in_room);
    if (room == NULL)
    {
        send_to_char("You can't do that here.\n", ch);
        return false;
    }

    // No pyres in or on water
    if (room->sector_type == SECT_WATER_NOSWIM || room->sector_type == SECT_WATER_SWIM || room->sector_type == SECT_UNDERWATER)
    {
        send_to_char("You could not possibly build a pyre with so much water around!\n", ch);
        return false;
    }

    // Make sure there is not already a pyre here
    if (lookupPyre(room) != NULL)
    {
        send_to_char("The energies of the pyre already here would disrupt your own.\n", ch);
        return false;
    }

    // Make sure the caster isn't on cooldown
    if (is_affected(ch, sn))
    {
        send_to_char("You are too drained from your last pyre to forge a new link to the Inferno.\n", ch);
        return false;
    }

    struct CallbackHandler 
    {
        static bool HandleMoveFrom(CHAR_DATA * ch, ROOM_INDEX_DATA * room, EchoAffect *, void *)
        {
            act("As you leave, your nascent pyre dwindles away to nothing.", ch, NULL, NULL, TO_CHAR);
            act("As $n leaves, $s nascent pyre dwindles away to nothing.", ch, NULL, NULL, TO_ROOM);
            return true;
        }

        static bool HandlePositionChange(CHAR_DATA * ch, int newPos, EchoAffect *, void *)
        {
            switch (newPos)
            {
                case POS_DEAD: 
                case POS_MORTAL:
                case POS_INCAP:
                case POS_STUNNED:
                case POS_SLEEPING:
                    act("As you lose consciousness, your nascent pyre dwindles away to nothing.", ch, NULL, NULL, TO_CHAR);
                    act("As $n loses consciousness, $s nascent pyre dwindles away to nothing.", ch, NULL, NULL, TO_ROOM);
                    return true;

                case POS_FIGHTING:
                    act("You break your concentration to fight, and your nascent pyre dwindles away to nothing.", ch, NULL, NULL, TO_CHAR);
                    act("$n breaks $s concentration to fight, and $s nascent pyre dwindles away to nothing.", ch, NULL, NULL, TO_ROOM);
                    return true;

            }

            return false;
        }

        static bool DamageCaster(CHAR_DATA * ch, EchoAffect *, void *) 
        {
            damage_new(ch, ch, dice(5, 20), TYPE_HIT, DAM_SLASH, true, "slice"); 
            return false;
        }

        static bool FinishSpell(CHAR_DATA * ch, EchoAffect *, void * tag)
        {
            int spellLevel = reinterpret_cast<int>(tag);
            // Sanity check
            if (ch->in_room == NULL)
            {
                std::ostringstream mess;
                mess << "Cannot finish bloodpyre; character " << ch->name << " not in room";
                bug(mess.str().c_str(), 0);
                send_to_char("There has been a problem with completing the bloodpyre spell; please inform the gods.\n", ch);
            }

            // Create the pyre object
            OBJ_DATA * obj(create_object(get_obj_index(OBJ_VNUM_PYRE), spellLevel));
            if (obj == NULL)
            {
                bug("Failed to load pyre object", 0);
                send_to_char("There has been a problem with creating a pyre; please inform the gods.\n", ch);
            }
            obj_to_room(obj, ch->in_room);

            // Build up the effect
            AFFECT_DATA af = {0};
            af.where    = TO_AFFECTS;
            af.type     = gsn_bloodpyre;
            af.level    = spellLevel;
            af.duration = number_range(40, 80) * 2;
            af.valid    = true;
            af.point    = new PyreInfo(obj);
            affect_to_char(ch, &af);
            return false;
        }
    };

    act("With a quick gesture and muttered word, you cause a small flame to flare into existence.", ch, NULL, NULL, TO_CHAR);
    act("With a quick gesture and muttered word, $n causes a small flame to flare into existence.", ch, NULL, NULL, TO_ROOM);
    
    // Apply the echo affect
    EchoAffect * echoAff(new EchoAffect(1));
    echoAff->SetTag(reinterpret_cast<void*>(level));
    echoAff->SetMoveCallback(&CallbackHandler::HandleMoveFrom);
    echoAff->SetPositionCallback(&CallbackHandler::HandlePositionChange);
    echoAff->AddLine(NULL,  
                    "Focusing your will on the fire, you begin the intonation. Slowly, you feel power building within you, drawn from the Inferno itself.", 
                    "$n begins to chant, softly at first, but growing louder and louder.");

    echoAff->AddLine(&CallbackHandler::DamageCaster, 
                    "As your chanting grows louder, you slice your own hand open! Droplets of blood fall into the growing pyre, where they spatter and hiss even as they vanish.",
                    "Suddenly, $n slices $s own hand open! Droplets of blood fall into the growing pyre, where they spatter and hiss even as they vanish.");

    echoAff->AddLine(&CallbackHandler::FinishSpell,
                    "With a final word you seal the spell; in front of you the pyre blazes brightly, the link between you almost palpable to your envigorated senses.",
                    "With a final word, $n finishes $s chant; in front of $m the pyre blazes brightly, crackling and popping with unmistakable energy.");

    EchoAffect::ApplyToChar(ch, echoAff);
    return true;
}

bool spell_heartoftheinferno(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    // Check for cooldown
    if (is_affected(ch, sn))
    {
        send_to_char("You are not yet ready to again draw upon the Heart of the Inferno.\n", ch);
        return false;
    }

    // Make sure the object is of type ruby
    OBJ_DATA * obj(static_cast<OBJ_DATA*>(vo));
    if (obj->material != material_lookup("ruby"))
    {
        send_to_char("Only rubies have the power to contain the Inferno's heart.\n", ch);
        return false;
    }

    act("You hold $p close to your mouth, murmuring softly to it as you draw deeply upon the powers of the Inferno.", ch, obj, NULL, TO_CHAR);
    act("$n holds $p close to $s mouth, murmuring softly to it.", ch, obj, NULL, TO_ROOM);

    // Get the existing affect, if any
    AFFECT_DATA af = {0};
    af.where        = TO_OBJECT;
    af.type         = sn;
    af.level        = UMAX(level, 55);
    af.duration     = -1;

    AFFECT_DATA * paf(get_obj_affect(obj, sn));
    if (paf == NULL)
    {
        // Add the base affect
        af.bitvector    = ITEM_NODESTROY | ITEM_NOLOCATE | ITEM_MAGIC | ITEM_HUM | ITEM_WARM;
        af.location     = APPLY_NONE;
        af.modifier     = 0;
        affect_to_obj(obj, &af);
        af.bitvector    = 0;
        paf = get_obj_affect(obj, sn);

        act("$p begins to glow softly, humming with obvious power as untold fiery energy washes over it.", ch, obj, NULL, TO_CHAR);
        act("$p begins to glow softly, humming with obvious power.", ch, obj, NULL, TO_ROOM);
    }
    else
    {
        // Make sure the correct affect is found
        while (paf->location != APPLY_NONE)
        {
            paf = get_obj_affect(obj, sn, paf);
            if (paf == NULL)
            {
                // Did not find the affect; this should not happen
                bug("Missing heart of the inferno primary affect", 0);
                send_to_char("An error has occurred, please contact the gods.\n", ch);
                return false;
            }
        }

        act("You sense the fiery core within $p pulse as its connection to the Inferno grows!", ch, obj, NULL, TO_CHAR);
        act("$p seems to glitter brightly for a moment.", ch, obj, NULL, TO_ROOM);
    }
   
    // Upgrade the affect and add any other modifiers
    ++paf->modifier;
    int skill = get_skill(ch, sn);
    if ((paf->modifier > 10 && paf->modifier % 2 == 0)
    ||  paf->modifier == 1 || paf->modifier == 5 || paf->modifier == 9)
    {
        // At levels 1, 5, 9, and all even levels after 10, add a damroll and possibly a hitroll
        af.location = APPLY_DAMROLL;
        af.modifier = 1;
        affect_to_obj(obj, &af);

        if (number_percent() < skill)
        {
            af.location = APPLY_HITROLL;
            af.modifier = 1;
            affect_to_obj(obj, &af);
        }
    }
    else if ((paf->modifier > 10 && paf->modifier % 2 == 1)
         ||  paf->modifier == 3 || paf->modifier == 7)
    {
        // At levels 3, 7, and all odd levels after 10, add [4-6] hp and [8 - 12] mana
        af.location = APPLY_HIT;
        af.modifier = 4 + (number_percent() < skill ? 1 : 0) + (number_percent() < (skill / 2) ? 1 : 0);
        affect_to_obj(obj, &af);

        af.location = APPLY_MANA;
        af.modifier = 8 + (number_percent() < skill ? 2 : 0) + (number_percent() < (skill / 2) ? 2 : 0);
        affect_to_obj(obj, &af);
    }

    // Apply 30-hour (real hour) cooldown
    AFFECT_DATA caf = {0};
    caf.where    = TO_AFFECTS;
    caf.type     = sn;
    caf.level    = level;
    caf.duration = 2 * 60 * 30;
    affect_to_char(ch, &caf);
   
    // Log this activity
    std::ostringstream mess;
    mess << ch->name << " just cast heart of the inferno on " << obj->short_descr << "; level is now " << paf->modifier;
    wiznet(const_cast<char*>(mess.str().c_str()), NULL, NULL, WIZ_ACTIVITY, 0, 0);
    log_string(mess.str().c_str());
    return true;
}

bool spell_heartfire(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    // Check for cooldown
    AFFECT_DATA * paf(get_affect(ch, sn));
    if (paf != NULL && paf->duration >= 0)
    {
        send_to_char("You are not yet ready to call the Inferno into your heart again.\n", ch);
        return false;
    }

    // Get the argument
    char arg[MAX_INPUT_LENGTH];
    one_argument(target_name, arg);

    // Check for starting
    if (strcmp(arg, "ignite") == 0)
    {
        if (paf == NULL)
        {
            // Add the effect
            AFFECT_DATA af = {0};
            af.where    = TO_AFFECTS;
            af.type     = sn;
            af.duration = -1;
            af.level    = level;
            af.modifier = 12;
            affect_to_char(ch, &af);
        }
        else
        {
            if (paf->modifier >= 36)
            {
                send_to_char("Your heart is pounding in your chest! You could not possibly sustain any more heartfire!\n", ch);
                return false;
            }

            // Intensify the effect
            paf->modifier += 12;
        }

        act("Your heart races as a wave of fiery energy pours into it!", ch, NULL, NULL, TO_CHAR);
        act("A wave of energy, nearly tangible in its strength, pulses once from $n.", ch, NULL, NULL, TO_ROOM);
        return true;
    }

    // Check for ending
    if (strcmp(arg, "extinguish") == 0)
    {
        if (paf == NULL)
        {
            send_to_char("Your heart is not presently aflame.\n", ch);
            return false;
        }

        // Convert the effect to a cooldown
        send_to_char("You close off your heart to the energy of the Inferno, and feel its fiery power leave you.\n", ch);
        paf->modifier = 0;
        paf->duration = 20 - (level / 17) - ((get_skill(ch, sn) - 75) / 5);
        return true;
    }

    send_to_char("You must specify whether to ignite or extinguish your heartfire.\n", ch);
    return false;
}

bool spell_heatmine(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    // Check for cooldown
    if (is_affected(ch, sn))
    {
        send_to_char("You are not yet ready to empower another brimstone ward.\n", ch);
        return false;
    }

    // Check for existing heatmine
    if (room_is_affected(ch->in_room, sn))
    {
        send_to_char("There is already a brimstone ward guarding this place.\n", ch);
        return false;
    }

    // No heatmines in or on water
    if (ch->in_room->sector_type == SECT_WATER_NOSWIM || ch->in_room->sector_type == SECT_WATER_SWIM || ch->in_room->sector_type == SECT_UNDERWATER)
    {
        send_to_char("Your brimstone ward would have no power in this place.\n", ch);
        return false;
    }

    struct CallbackHandler
    {
        static bool HandleMoveFrom(CHAR_DATA * ch, ROOM_INDEX_DATA * room, EchoAffect * thisAffect, void * tag)
        {
            send_to_char("You sense the forces gathering about the circle disperse as you abandon the ritual.\n", ch);
            return true;
        }

        static bool HandlePositionChange(CHAR_DATA * ch, int newPos, EchoAffect * thisAffect, void * tag)
        {
            if (newPos != POS_STANDING)
            {
                send_to_char("You sense the forces gathering about the circle disperse as you abandon the ritual.\n", ch);
                return true;
            }
            return false;
        }

        static bool FinishSpell(CHAR_DATA * ch, EchoAffect *, void * tag)
        {
            int level(reinterpret_cast<int>(tag));

            // Add the affect
            AFFECT_DATA af = {0};
            af.where     = TO_ROOM;
            af.type      = gsn_heatmine;
            af.level     = level;
            af.duration  = 7 + (level / 10);
            affect_to_room(ch->in_room, &af);

            // Add a cooldown
            af.where     = TO_AFFECTS;
            af.duration  = UMIN(12, 24 - ((get_skill(ch, gsn_heatmine) - 76) / 2));
            affect_to_char(ch, &af);
            return false;
        }
    };

    // Do initial echo
    act("With two fingers you carefully trace a large horizontal circle, even as you begin a low chant deep in your throat.", ch, NULL, NULL, TO_CHAR);
    act("With two fingers $n carefully traces a large horizontal circle, even as $e begins a low chant deep in $s throat.", ch, NULL, NULL, TO_ROOM);

    // Set up the echo affect
    EchoAffect * echoAff(new EchoAffect(1));
    echoAff->SetTag(reinterpret_cast<void*>(level));
    echoAff->SetMoveCallback(&CallbackHandler::HandleMoveFrom);
    echoAff->SetPositionCallback(&CallbackHandler::HandlePositionChange);
    if (ch->race == global_int_race_shuddeni)
    {
        echoAff->AddLine(NULL,
                        "You finish the circle and concentrate briefly, willing fiery energy into the ring as you continue to chant.",
                        "$n finishes the circle and concentrates briefly, $s low chant never ceasing.");
    }
    else
    {
        echoAff->AddLine(NULL,
                        "You finish the circle and close your eyes briefly, willing fiery energy into the ring as you continue to chant.",
                        "$n finishes the circle and closes $s eyes in concentration, $s low chant never ceasing.");
    }

    echoAff->AddLine(&CallbackHandler::FinishSpell,
                    "A frisson of energy dances around the perimeter of the circle, empowering it as you complete the ritual.",
                    "$n's chant abates as $s traced circle begins to slowly burn!");

    EchoAffect::ApplyToChar(ch, echoAff);
    return true;
}

bool spell_wrathofanakarta(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    // Check for cooldown
    if (is_affected(ch, sn))
    {
        send_to_char("You aren't yet ready to channel such potent wrath again.\n", ch);
        return false;
    }

    act("You open your mind to the ravages of An'Akarta, and feel a burning hatred envelop you!", ch, NULL, NULL, TO_CHAR);
    act("A look of pure fury transfixes $n's face as $e glares on the world with intense hatred!", ch, NULL, NULL, TO_ROOM);

    // Add the affect
    AFFECT_DATA af = {0};
    af.where    = TO_AFFECTS;
    af.type     = sn;
    af.duration = (level / 10) + ((get_skill(ch, sn) - 75) / 5);
    af.level    = level;
    affect_to_char(ch, &af);
    return true;
}

bool spell_flameunity(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    // Make sure there is a fire here
    OBJ_DATA * obj(lookupGroundFireHere(ch));
    if (obj == NULL)
    {
        send_to_char("There are no suitable flames here.\n", ch);
        return false;
    }

    act("You speak a word of power and almost immediately melt into $p.", ch, obj, NULL, TO_CHAR);
    act("Uttering a word of power, $n seems to almost dissolve into wisps of flame, merging with $p.", ch, obj, NULL, TO_ROOM);

    // Add the effect
    AFFECT_DATA af = {0};
    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = -1;
    af.point     = obj;
    af.location  = APPLY_NONE;
    affect_to_char(ch, &af);
    return true;
}

void checkConflagration(CHAR_DATA * ch)
{
    // Check for disqualifiers
    if (ch->in_room == NULL || room_is_affected(ch->in_room, gsn_blaze) 
    || ch->in_room->sector_type == SECT_WATER_NOSWIM || ch->in_room->sector_type == SECT_WATER_SWIM || ch->in_room->sector_type == SECT_UNDERWATER)
        return;
    
    // Check odds; 50-75%, depending on skill
    if (number_percent() > 50 + UMAX(0, (get_skill(ch, gsn_conflagration) - 75)))
        return;

    // Add blaze effect
    send_to_char("A furious blaze bursts from you, setting this place on fire!\n", ch);
    act("A furious blaze bursts from $n, setting this place on fire!", ch, NULL, NULL, TO_ROOM);

    AFFECT_DATA af = {0};
    af.where     = TO_ROOM;
    af.type      = gsn_blaze;
    af.level     = ch->level;
    af.duration  = ch->level / 5;
    affect_to_room(ch->in_room, &af);
}

bool spell_conflagration(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    // Check for cooldown
    if (is_affected(ch, sn))
    {
        send_to_char("You are not yet ready to spread another conflagration.\n", ch);
        return false;
    }

    // Cause a blaze in the room
    send_to_char("You feel an aura of heat take up residence within you.\n", ch);
    checkConflagration(ch);

    // Add the effect
    AFFECT_DATA af = {0};
    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = 2;
    af.location  = APPLY_NONE;
    affect_to_char(ch, &af);
    return true;
}

bool spell_aspectoftheinferno(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    // Check for cooldown
    if (is_affected(ch, sn))
    {
        send_to_char("You are not prepared to once again assume the aspect of the Inferno.\n", ch);
        return false;
    }

    act("With a screamed word of power, you call forth the burning power of the Inferno to fill you!", ch, NULL, NULL, TO_CHAR);
    act("The world grows momentarily hazy as the fiery energy rushes into you, quickening your heart and enflaming your blood!", ch, NULL, NULL, TO_CHAR);
    act("$n screams a word of power, and the area seems suddenly cooler as massive amounts of heat rush into $m!", ch, NULL, NULL, TO_ROOM);

    // Add the effect
    AFFECT_DATA af = {0};
    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = 6 + UMAX(0, (level - 48)) + UMAX(0, ((get_skill(ch, sn) - 70) / 10));
    af.location  = APPLY_NONE;
    affect_to_char(ch, &af);
    return true;
}

bool spell_baptismoffire( int sn, int level, CHAR_DATA *ch, void *vo,int target)
{
    act("$n calls pure fire into $s body to cleanse $m of $s ills!", ch, NULL, NULL, TO_ROOM);
    act("You call pure fire into your body to cleanse you of your ills!", ch, NULL, NULL, TO_CHAR);
    affect_strip(ch, gsn_poison);
    affect_strip(ch, gsn_pox);
    affect_strip(ch, gsn_plague);
    affect_strip(ch, gsn_cloudkill);
    affect_strip(ch, gsn_petrify);
    affect_strip(ch, gsn_encase);
    affect_strip(ch, gsn_freeze);
    affect_strip(ch, gsn_frostbite);
    affect_strip(ch, gsn_winterwind);

    if (IS_AFFECTED(ch, AFF_POISON))
	REMOVE_BIT(ch->affected_by, AFF_POISON);
    if (IS_AFFECTED(ch, AFF_PLAGUE))
        REMOVE_BIT(ch->affected_by, AFF_PLAGUE);
    
    damage_old( ch, ch, number_range(level, level*1.5), sn, DAM_FIRE, TRUE);
    return TRUE;
}

bool spell_beam_of_fire( int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;

    // Do basic beam of fire
    std::vector<DamageInfo> damage(1);
    damage[0].type = DAM_FIRE;
    damage[0].amount = dice(level, 6) + 50;
    if (saves_spell(level, ch, victim, DAM_FIRE))
        damage[0].amount /= 2;

    // Check for holy flame
    if (number_percent() <= get_skill(ch, gsn_holyflame))
    {
        check_improve(ch, victim, gsn_holyflame, true, 4);
        damage.push_back(DamageInfo(10 + dice(2, level / 2), DAM_HOLY));
        if (saves_spell(level, ch, victim, DAM_HOLY))
            damage[1].amount /= 2;
    }
    else
        check_improve(ch, victim, gsn_holyflame, false, 4);

    damage_new(ch, victim, damage, sn, true);
    return true;
}

bool spell_blast( int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *vch, *vch_next;
    int x, dam;
    char buf[MAX_STRING_LENGTH];

    if (!ch->in_room)
	return FALSE;

    send_to_char("You send a powerful blast of flame sweeping through the area.\n\r", ch);
    act("A powerful blast of flame explodes from around $n!", ch, NULL, NULL, TO_ROOM);

    for (vch = ch->in_room->people; vch; vch = vch_next)
    {
        vch_next = vch->next_in_room;

        if (!is_same_group(ch, vch) && !is_safe_spell(ch, vch, TRUE) && (!IS_NPC(vch) || !IS_AFFECTED(vch, AFF_WIZI)))
        {
            if (!IS_NPC(vch) && !vch->fighting)
            {
                if (can_see(vch, ch))
                     sprintf(buf, "Help!  %s is attacking me!", PERS(ch, vch));
                else
                     sprintf(buf, "Help!  Someone is attacking me!");
                do_autoyell(vch, buf);
                check_killer(ch, vch);
            }
            
            dam = dice(level, 5) + level / 3;
            damage_old(ch, vch, dam, sn, DAM_FIRE, TRUE);
        }
    }

    for (x = 0; x < 6; x++)
	if (ch->in_room->exit[x]
	 && !IS_SET(ch->in_room->exit[x]->exit_info, EX_CLOSED)
         && !IS_SET(ch->in_room->exit[x]->exit_info, EX_WALLED)
         && !IS_SET(ch->in_room->exit[x]->exit_info, EX_ICEWALL)
         && !IS_SET(ch->in_room->exit[x]->exit_info, EX_FAKE)
         && !(room_is_affected(ch->in_room->exit[x]->u1.to_room, gsn_wallofwater))	
         ) 
         {
                sprintf(buf, "A blast of flame explodes into the room from %s!",
                        (x == 0 ? "the south" :
                         x == 1 ? "the west"  :
                         x == 2 ? "the north" :
                         x == 3 ? "the east"  :
                         x == 4 ? "below"     : "above" ));
		act(buf, ch->in_room->exit[x]->u1.to_room->people, NULL, NULL, TO_CHAR);
                act(buf, ch->in_room->exit[x]->u1.to_room->people, NULL, NULL, TO_ROOM);

 	    for (vch = ch->in_room->exit[x]->u1.to_room->people; vch; vch = vch_next)
    	    {
        	vch_next = vch->next_in_room;
		if (!is_same_group(ch, vch)
	 	 && !is_safe_spell(ch, vch, TRUE)
	 	 && (!IS_NPC(vch) || !IS_AFFECTED(vch, AFF_WIZI)))
		{
	    	    if (!IS_NPC(vch) && !vch->fighting)
	    	    {
			if (can_see(vch, ch))
		     	    sprintf(buf, "Help!  %s is attacking me!", PERS(ch, vch));
			else
		     	    sprintf(buf, "Help!  Someone is attacking me!");
			do_autoyell(vch, buf);
	    	    }
	    
    		    if (IS_NPC(vch) && ((IS_SET(vch->act, ACT_SENTINEL) || IS_SET(vch->act, ACT_NOTRACK))) && (vch->hit < vch->max_hit))
        		dam = 0;
		    else
	    	        dam = dice(level, 4);

	    	    damage_old(ch, vch, dam, sn, DAM_FIRE, TRUE);
		    vch->tracking = ch;
		    if (IS_NPC(vch))
			add_tracks(ch->in_room->exit[x]->u1.to_room, ch, OPPOSITE(x));
		}
	   }
   	   if (IS_SET(ch->in_room->exit[x]->exit_info, EX_WALLOFVINES))
            {
                REMOVE_BIT(ch->in_room->exit[x]->exit_info, EX_WALLOFVINES);
                
                if (ch->in_room->exit[x]->u1.to_room->exit[OPPOSITE(x)]
                 && (ch->in_room->exit[x]->u1.to_room->exit[OPPOSITE(x)]->u1.to_room == ch->in_room)
		 && IS_SET(ch->in_room->exit[x]->u1.to_room->exit[OPPOSITE(x)]->exit_info, EX_WALLOFVINES))
                {
                    REMOVE_BIT(ch->in_room->exit[x]->u1.to_room->exit[OPPOSITE(x)]->exit_info, EX_WALLOFVINES);
                    
                    sprintf(buf, "As the heat from the flames blasts through the area, the wall of vines %s bursts into flames and is consumed!",
			((OPPOSITE(x) == 0) ? "to the north" : (OPPOSITE(x) == 1) ? "to the east" :
			 (OPPOSITE(x) == 2) ? "to the south" : (OPPOSITE(x) == 3) ? "to the west" :
			 (OPPOSITE(x) == 4) ? "above you" : "below you"));
		    act(buf, ch->in_room->exit[x]->u1.to_room->people, NULL, NULL, TO_CHAR);
		    act(buf, ch->in_room->exit[x]->u1.to_room->people, NULL, NULL, TO_ROOM);
		}

                sprintf(buf, "As the heat from the flames blasts through the area, the wall of vines %s bursts into flames and is consumed!",
		    ((x == 0) ? "to the north" : (x == 1) ? "to the east" :
		     (x == 2) ? "to the south" : (x == 3) ? "to the west" :
		     (x == 4) ? "above you" : "below you"));
		act(buf, ch->in_room->people, NULL, NULL, TO_CHAR);
		act(buf, ch->in_room->people, NULL, NULL, TO_ROOM);
	}		    
    }

    return TRUE;
}	  

bool spell_blaze(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
    CHAR_DATA *vch, *vch_next;
    int dam;
    char buf[MAX_STRING_LENGTH];

    if (!ch->in_room)
	return FALSE;

    if (room_is_affected(ch->in_room, gsn_blaze))
    {
	send_to_char("This area is already filled with blazing flames.\n\r", ch);
	return FALSE;
    }

    send_to_char("A furious blaze erupts throughout the area!\n\r", ch);
    act("A furious blaze erupts throughout the area!", ch, NULL, NULL, TO_ROOM);

    af.where	 = TO_ROOM;
    af.type	 = sn;
    af.level	 = level;
    af.duration  = level/5;
    af.modifier  = 0;
    af.location  = 0;
    af.bitvector = 0;
    affect_to_room(ch->in_room, &af);

    for (vch = ch->in_room->people; vch; vch = vch_next)
    {
	vch_next = vch->next_in_room;

	if (!is_same_group(ch, vch)
	 && !is_safe_spell(ch, vch, TRUE)
	 && (!IS_NPC(vch) || !IS_AFFECTED(vch, AFF_WIZI)))
	{
	    if (!IS_NPC(vch) && !vch->fighting)
	    {
		if (can_see(vch, ch))
		    sprintf(buf, "Help!  %s is attacking me!", PERS(ch, vch));
		else
		    sprintf(buf, "Help!  Someone is attacking me!");	
		check_killer(ch, vch);
	   	do_autoyell(vch, buf);
	    }

	    dam = dice(level, 4) + level / 4;
	    damage(ch, vch, dam, gsn_blaze, DAM_FIRE, TRUE);
	}
    }

    return TRUE;
}

bool spell_blazinginferno( int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;


        if (is_affected(ch, gsn_blazinginferno))
        {
        send_to_char("You have too recently started a blazing inferno.\n\r", ch);
        return FALSE;
        }

        if (!ch->in_room)
                return FALSE;

        if (area_is_affected(ch->in_room->area, gsn_icestorm))
        {
        send_to_char("A blaze lights up, but is quickly stiffled by the ice storm.\n\r", ch);
        return TRUE;
        }

        if (ch->in_room->sector_type == SECT_WATER_NOSWIM || ch->in_room->sector_type == SECT_WATER_SWIM || ch->in_room->sector_type == SECT_UNDERWATER)
                {
                send_to_char("You can't start a fire here!\n\r", ch);
                return FALSE;
                }
        if (room_is_affected(ch->in_room,gsn_wallofwater))
        {
            send_to_char("The wall of water extinguishes your attempts at an inferno.\n\r",ch);
            return TRUE;
        }

        if (room_is_affected(ch->in_room, gsn_blazinginferno))
        {
            send_to_char("This room is already blazing!\n\r", ch);
            return FALSE;
        }

        af.where        = TO_ROOM_AFF;
        af.type = sn;
        af.level        = level;
        af.duration     = 2;
        af.location     = 0;
        af.modifier     = 0;
        af.bitvector   = 0;
        affect_to_room(ch->in_room, &af);
        af.duration     = 120;
        affect_to_char(ch, &af);

        act("In a resounding roar, the room explodes in a blaze!", ch, NULL, NULL, TO_CHAR);
        act("In a resounding roar, the room explodes in a blaze!", ch, NULL, NULL, TO_ROOM);
    return TRUE;
}

bool spell_burning_hands(int sn,int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    static const int dam_each[] =
    {
         0,
         0,  0,  0,  0, 14,     17, 20, 23, 26, 29,
        29, 29, 30, 30, 31,     31, 32, 32, 33, 33,
        34, 34, 35, 35, 36,     36, 37, 37, 38, 38,
        39, 39, 40, 40, 41,     41, 42, 42, 43, 43,
        44, 44, 45, 45, 46,     46, 47, 47, 48, 48
    };
    int dam;

    level       = UMIN(level, static_cast<int>(sizeof(dam_each)/sizeof(dam_each[0])) - 1);
    level       = UMAX(0, level);
    dam         = number_range( dam_each[level] / 2, dam_each[level] * 2 );
    if ( saves_spell( level, ch, victim,DAM_FIRE) )
        dam /= 2;
    damage_old( ch, victim, dam, sn, DAM_FIRE,TRUE);
    return TRUE;
}

bool spell_burnout( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if (is_affected(ch, sn))
    {
	if (IS_OAFFECTED(ch, AFF_BURNOUT))
	    send_to_char("Your body is already overcharged.\n\r", ch);
	else
	    send_to_char("You are currently recovering from a previous burnout.\n\r", ch);
	return FALSE;
    }

    af.where	 = TO_OAFFECTS;
    af.type	 = sn;
    af.level	 = level;
    af.duration  = (level * 2) / 15;
    af.location  = 0;
    af.modifier  = 0;
    af.bitvector = AFF_BURNOUT;
    affect_to_char(ch, &af);

    send_to_char("Your body surges with regenerative energy.\n\r", ch);

    return TRUE;
}

bool savesConsumeSpecial(CHAR_DATA * ch, CHAR_DATA * victim)
{
    // Check firekin
    if (number_percent() < (get_skill(victim, gsn_firekin) / 2))
    {
        check_improve(victim, ch, gsn_firekin, TRUE, 1);
        return true;
    }

    // Check flameheart    
    if (number_percent() < (get_skill(victim, gsn_flameheart)))
    {
        check_improve(victim, ch, gsn_flameheart, TRUE, 1);
        return true;
    }

    return false;
}

bool spell_consume( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;
    af.valid = TRUE;
    af.point = NULL;

    if (is_affected(victim, gsn_consume))
    {
        send_to_char("Flame already consumes them.\n\r", ch);
        return FALSE;
    }

    if (savesConsumeSpecial(ch, victim) || saves_spell(level, ch, victim, DAM_FIRE))
    {
        if (ch != victim)
          send_to_char("Your skin begins to burn, but it passes.\n\r",victim);
        send_to_char("They resist your consuming fires.\n\r", ch);
        return TRUE;
    }

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = level/5;
    af.location  = 0;
    af.modifier  = 0;
    af.bitvector = 0;
    affect_to_char(victim,&af);

    act("$n screams as $s skin begins to glow hot!", victim, NULL, NULL, TO_ROOM);
    act("You scream as your skin begins to glow hot!", victim, NULL, NULL, TO_CHAR);
    return TRUE;
}

bool spell_consuming_rage(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if (is_affected(ch, sn))
    {
	send_to_char("You are already filled with bloodlust!\n\r", ch);
	return FALSE;
    }

    af.where	 = TO_AFFECTS;
    af.type	 = sn;
    af.level	 = level;
    af.duration  = level / 5;
    af.location  = APPLY_DAMROLL;
    af.modifier  = 0;
    af.bitvector = 0;
    affect_to_char(ch, &af);

    send_to_char("Your breathing quickens as you are consumed with rage!\n\r", ch);

    return TRUE;
}

bool spell_delayedblastfireball( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
        AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
        CHAR_DATA *fbmob;
        int wait = -1;

        if (target_name[0] == '\0')
        {
	        //send_to_char("Place a delayed blast fireball to wait how long?\n\r", ch);
	        //return FALSE;
		wait = number_range(0, 5);
        }

        if (IS_OAFFECTED(ch, AFF_GHOST))
        {
        send_to_char("You can't place a delayed blast fireball when you're a ghost.\n\r", ch);
        return FALSE;
        }

        if (wait < 0 && (wait = atoi(target_name)) < 0)
        {
        send_to_char("Place a delayed blast fireball to wait how long?\n\r", ch);
        return FALSE;
        }
	
        if (is_affected(ch, gsn_delayedblastfireball) && get_modifier(ch->affected, gsn_delayedblastfireball) >= ch->level/17)
        {
        send_to_char("You can't yet place another delayed blast fireball.\n\r",
ch);
        return FALSE;
        }

        if (!ch->in_room)
                return FALSE;

        if (ch->in_room->sector_type == SECT_WATER_NOSWIM || ch->in_room->sector_type == SECT_WATER_SWIM || ch->in_room->sector_type == SECT_UNDERWATER)
        {
        send_to_char("You can't place a fireball with all this water here.\n\r", ch);
        return FALSE;
        }

        fbmob = create_mobile(get_mob_index(MOB_VNUM_DELAYED_FIREBALL));
        fbmob->level = ch->level;
        fbmob->mobvalue[0] = wait*7;
        char_to_room(fbmob, ch->in_room);
        prog_focus(fbmob, NULL, NULL, ch->name, MOB_PROG);

        act("$n lifts $s arms, and a sphere of fire forms in front of $m, then sinks into the ground.", ch, NULL, NULL, TO_ROOM);
        act("You summon the energy of a fireball, channeling it into the ground to wait.", ch, NULL, NULL, TO_CHAR);


    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = wait+5;
    af.location  = 0;
    af.modifier  = is_affected(ch, gsn_delayedblastfireball) ? get_modifier(ch->affected, gsn_delayedblastfireball) + 1 : 1;
    af.bitvector = 0;
    affect_strip(ch, gsn_delayedblastfireball);
    affect_to_char(ch, &af);
    return TRUE;
}


bool spell_disintegration( int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    int chance;
    char buf[MAX_STRING_LENGTH];

    if (is_affected(ch, gsn_disintegration))
    {
        send_to_char("You are not ready to disintegrate anyone again yet.\n\r",
ch);
        return FALSE;
    }

    act("$n calls forth a beam of pure energy to disintegrate $N!", ch, NULL, victim, TO_NOTVICT);
    act("$n calls forth a beam of pure energy to disintegrate you!", ch, NULL, victim, TO_VICT);
    act("You call forth a beam of pure energy to disintegrate $N!", ch, NULL, victim, TO_CHAR);

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = 12;
    af.modifier  = 0;
    af.location  = 0;
    af.bitvector = 0;
    affect_to_char( ch, &af );

    if (!IS_NPC(victim) || !IS_SET(victim->act, ACT_NOSUBDUE))
    {
        if ((chance = number_percent()) <= 2)
        {
            act("$n loses control of $s spell, and screams as $e is reduced to a smoldering corpse.", ch, NULL, NULL, TO_ROOM);
            act("You lose control of your spell, and scream as you are reduced to a smoldering corpse.", ch, NULL, NULL, TO_CHAR);
	    sprintf(buf,"%s lost control of disintegrate in %s [%i].\n\r",ch->name,ch->in_room->name,ch->in_room->vnum);
     	    log_string(buf);
            raw_kill(ch);
            return TRUE;
        }
        else if (chance < 8)
        {
            act("$n is struck by the beam of energy, and dies instantly as it sears through $m.", victim, NULL, NULL, TO_ROOM);
            act("You are struck by the beam of energy, and die instantly as it sears through you.", victim, NULL, NULL, TO_CHAR);
            kill_char(victim, ch);
            return TRUE;
        }
    }

    damage_old( ch, victim, 325, sn,DAM_FIRE,TRUE);
    return TRUE;
}

bool spell_dispersion( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
    ROOM_INDEX_DATA *pRoomIndex;
    char buf[MAX_INPUT_LENGTH];
    CHAR_DATA *vch;
    CHAR_DATA *vch_next;

    if (is_affected(ch, sn))
    {
        send_to_char("You are not prepared to disperse again just yet.\n", ch);
        return false;
    }

    if ( ch->in_room == NULL
    ||   IS_SET(ch->in_room->room_flags, ROOM_NO_RECALL)
    ||   IS_SET(ch->in_room->room_flags, ROOM_NOGATE)
    ||   is_affected(ch, gsn_matrix)
    || ( ch->in_room->area->area_flags & AREA_UNCOMPLETE )
    || ( ch != ch && IS_SET(ch->imm_flags,IMM_SUMMON))
    || (ch->move == 0))
    {
        send_to_char( "You cannot disperse from here.\n\r", ch );
        return FALSE;
    }

    pRoomIndex = get_random_room(ch);

    act( "$n explodes in flame and is gone!", ch, NULL, NULL, TO_ROOM );
    act( "You explode in flame and disappear!", ch, NULL, NULL, TO_CHAR);

    // Apply a cooldown if fighting
    if (ch->position == POS_FIGHTING)
    {
        AFFECT_DATA af = {0};
        af.where    = TO_AFFECTS;
        af.type     = sn;
        af.level    = level;
        af.duration = 16 - UMAX(0, (get_skill(ch, sn) - 70) / 5);
        affect_to_char(ch, &af);
    }

    ch->move /= 2;

    int baseDam = 30 + ((level - 1) * 2);

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
                if ((ch->fighting != vch) && !IS_NPC(vch) && vch->fighting!=ch)
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
                int dam = number_range( baseDam / 2, baseDam * 2 );
                   if ( saves_spell( level, ch, vch, DAM_FIRE) )
                        dam /= 2;

                damage_old( ch, vch, saves_spell( level,ch, vch,DAM_FIRE)
                ? dam / 2 : dam, sn,DAM_FIRE,TRUE);
            }
            continue;
        }
    }
    char_from_room( ch );
    char_to_room( ch, pRoomIndex );
    act( "$n pops into existence.", ch, NULL, NULL, TO_ROOM );
    do_look( ch, "auto" );

    if (ch->familiar != NULL && ch->familiar->in_room == ch->in_room)
        {
    act( "$n vanishes!", ch->familiar, NULL, NULL, TO_ROOM );
    char_from_room( ch->familiar );
    char_to_room( ch->familiar, pRoomIndex );
    act( "$n slowly fades into existence.", ch->familiar, NULL, NULL, TO_ROOM );    do_look( ch->familiar, "auto" );
        }

    return TRUE;
}


bool spell_etherealblaze( int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *vch;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if (!ch->in_room)
        return FALSE;

    if (room_is_affected(ch->in_room, gsn_etherealblaze))
    {
        send_to_char("This area is already covered in revealing flames.\n\r", ch);
        return FALSE;
    }

    if (is_affected(ch, gsn_etherealblaze))
    {
        send_to_char("You are not prepared to call another ethereal blaze yet.\n\r", ch);
        return FALSE;
    }

    send_to_char("Eerie flames begin to lick at your surroundings.\n\r", ch);
    act("Eerie flames begin to lick at your surroundings.", ch, NULL, NULL, TO_ROOM);

    for (vch = ch->in_room->people; vch; vch = vch->next_in_room)
        if (IS_AFFECTED(vch, AFF_HIDE) || is_affected(vch, gsn_camouflage) || IS_OAFFECTED(vch, AFF_SHADOWMASTERY))
        {
             send_to_char("The illuminating flames reveal your location.\n\r", vch);
             uncamo_char(vch);
             unhide_char(vch);
             REMOVE_BIT(vch->oaffected_by, AFF_SHADOWMASTERY);
             affect_strip(vch, gsn_shadowmastery);
        }

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.location  = APPLY_NONE;
    af.modifier  = 0;
    af.duration  = level / 5;
    af.bitvector = 0;
    affect_to_room(ch->in_room, &af);

    af.duration  = level / 4;
    affect_to_char(ch, &af);

    return TRUE;
}

void applyCoronalGlow(CHAR_DATA * ch, CHAR_DATA * victim, int level, int duration, int amount)
{
    // Double the effect for WreathOfFlame
    if (getPyreInfoEffect(ch, PyreInfo::Effect_WreathOfFlame) != NULL)
        amount *= 2;
        
    // Choose echoes based on whether this is a new effect
    if (is_affected(victim, gsn_coronalglow))
    {
        send_to_char("The coronal glow about you grows brighter!\n", victim);
        act("The coronal glow about $n grows brighter!", victim, NULL, NULL, TO_ROOM);
    }
    else
    {
        send_to_char("You are surrounded by a coronal glow.\n", victim);
        act("$n is surrounded by a coronal glow.", victim, NULL, NULL, TO_ROOM);
    }
    
    // Apply the effect
    AFFECT_DATA af = {0};
    af.where     = TO_AFFECTS;
    af.type      = gsn_coronalglow;
    af.level     = level;
    af.duration  = duration;
    af.location  = APPLY_AC;
    af.modifier  = amount;
    af.bitvector = AFF_FAERIE_FIRE;
    affect_to_char(victim, &af);
    unhide_char(victim);
    uncamo_char(victim);
}

bool spell_coronal_glow( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;

    if ( IS_AFFECTED(victim, AFF_FAERIE_FIRE) )
    {
        act("$N is already aflame.", ch, NULL, victim, TO_CHAR);
        return FALSE;
    }

    if (saves_spell(level, ch, victim, DAM_FIRE))
    {
        act("A coronal glow begins to surround $n, but quickly fades.", victim, NULL, NULL, TO_ROOM);
	act("A coronal glow begins to surround you, but quickly fades.", victim, NULL, NULL, TO_CHAR);
	return TRUE;
    }

    applyCoronalGlow(ch, victim, level, level, 2 * level); 
    return TRUE;
}

bool spell_fireball( int sn, int level,CHAR_DATA *ch,void *vo,int target)
{
    char buf[MAX_INPUT_LENGTH];
    CHAR_DATA *victim = NULL;
    CHAR_DATA *vch;
    CHAR_DATA *vch_next;
    ROOM_INDEX_DATA *troom = ch->in_room;
    if (target == TARGET_CHAR)
    {
	victim = (CHAR_DATA *) vo;
        troom = victim->in_room;
    }

    int baseDam = 30 + ((level - 1) * 2);

    send_to_char( "Your fireball engulfs the room!\n\r", ch );
    if (victim)
    {
        if (!IS_AFFECTED(ch, AFF_WIZI) || ch->pIndexData->vnum != MOB_VNUM_DELAYED_FIREBALL)
	    act( "$n conjures a ball of fire to engulf the room!", ch, NULL, victim, TO_VICTROOM );
        else
            act( "$n engulfs the room!", ch, NULL, victim, TO_VICTROOM );
    }
    else
    {
        if (!IS_AFFECTED(ch, AFF_WIZI) || ch->pIndexData->vnum != MOB_VNUM_DELAYED_FIREBALL)
	    act( "$n conjures a ball of fire to engulf the room!", ch, NULL, NULL, TO_ROOM );
        else
            act( "$n engulfs the room!", ch, NULL, NULL, TO_ROOM );
    }

    for ( vch = char_list; vch != NULL; vch = vch_next )
    {
        vch_next        = vch->next;
        if ( vch->in_room == NULL )
            continue;
        if ((vch->in_room == troom) && (ch!=vch) 
	  && !(is_same_group(ch, vch)) && !is_safe_spell(ch,vch,TRUE) 
	  && (!IS_NPC(ch) || ch->pIndexData->vnum != MOB_VNUM_DELAYED_FIREBALL
            || (ch->level - vch->level < 9 && vch->level - ch->level < 9)))
        {
            if (vch != ch && !IS_IMMORTAL(vch) && !IS_OAFFECTED(vch,AFF_GHOST))
            {
                if ((ch->fighting != vch) && !IS_NPC(vch) && vch->fighting!=ch)
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
           	int dam = number_range( baseDam / 2, baseDam * 2 );
                if ( saves_spell( level, ch, vch, DAM_FIRE) )
                    dam /= 2;

                damage_old( ch, vch, dam, sn,DAM_FIRE,TRUE);
            }
            continue;
        }
    }

    checkMeltIcyPrisons(troom);
    return TRUE;
}

bool spell_fireblast(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    int direction;
    EXIT_DATA *pexit;
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    int dam;

    bool flying(is_flying(victim));
    dam = dice(level, 3 + (flying ? 2 : 0));
    if ( saves_spell( level, ch, victim, DAM_FIRE ) )
        dam /= 2;

    act ("$n sends a column of fire smashing into $N!", ch,NULL,victim,TO_NOTVICT);
    act ("You send a column of fire smashing into $N!", ch,NULL,victim,TO_CHAR);    act ("$n sends a column of fire smashing into you!", ch,NULL,victim,TO_VICT);
    damage_old( ch, victim, dam, sn,DAM_FIRE,TRUE);


    if (!victim || !victim->valid || IS_OAFFECTED(victim, AFF_GHOST))
        return FALSE;

    if (!saves_spell(level, ch, victim, DAM_BASH) || (flying && number_percent() < 50))
    {
        act("$n is stunned by the force of the blast!", victim, NULL, NULL, TO_ROOM);
        act("You are stunned by the force of the blast!", victim, NULL, NULL, TO_CHAR);
        WAIT_STATE(victim, UMAX(victim->wait, 2*PULSE_VIOLENCE+1));
    }

    if (is_affected(victim, gsn_anchor))
	return TRUE;

    if ((pexit = ch->in_room->exit[direction = number_range(0, 5)]) == NULL)
        return TRUE;
    else if (!IS_NPC(victim) || (!IS_SET(victim->act, ACT_NOSUBDUE) && !IS_SET(victim->act, ACT_SENTINEL)))
        {
        act("$n is smashed away by the blast of fire!", victim, NULL, NULL, TO_ROOM);
        act("You are crushed by the blast of fire!", victim, NULL, NULL, TO_CHAR);
        move_char(victim, direction, FALSE);
        }

    return TRUE;
}

bool spell_firebolt( int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim;
    char arg[MAX_INPUT_LENGTH];
    char *direction;
    ROOM_INDEX_DATA *pRoom = NULL;
    int dir = -1, dam, i;

    if (!ch->in_room)
        return FALSE;

    direction = one_argument(target_name, arg);

    if ((arg[0] == '\0') && !ch->fighting)
    {
        send_to_char("Who are you trying to firebolt?\n\r", ch);
        return FALSE;
    }

    if (direction[0] == '\0')
        pRoom = ch->in_room;
    else
    {
        if (ch->fighting)
        {
            send_to_char("You can't shoot a firebolt into another room while fighting!\n\r", ch);
            return FALSE;
        }

        for (i = 0; i < 6; i++)
            if (!str_prefix(direction, dir_name[i]))
                dir = i;

        if (dir == -1)
        {
            send_to_char("Invalid direction.\n\r",ch);
            return FALSE;
        }

        if (!ch->in_room->exit[dir]
         || IS_SET(ch->in_room->exit[dir]->exit_info, EX_CLOSED)
         || IS_SET(ch->in_room->exit[dir]->exit_info, EX_WALLED)
         || IS_SET(ch->in_room->exit[dir]->exit_info, EX_ICEWALL)
         || IS_SET(ch->in_room->exit[dir]->exit_info, EX_WALLOFFIRE)
         || IS_SET(ch->in_room->exit[dir]->exit_info, EX_WALLOFVINES)
         || IS_SET(ch->in_room->exit[dir]->exit_info, EX_FAKE))
        {
            send_to_char("You can't shoot a firebolt in that direction!\n\r",ch);
            return FALSE;
        }

        pRoom = ch->in_room->exit[dir]->u1.to_room;
    }

    if (pRoom == NULL)
    {
        send_to_char("You cannot shoot your firebolt in that direction!\n\r",ch);        return FALSE;
    }

    if (room_is_affected(ch->in_room, gsn_smoke)
     || room_is_affected(pRoom, gsn_smoke))
    {
        send_to_char("The thick smoke wafting by blocks you from targeting your firebolt.\n\r",ch);
        return FALSE;
    }

    if (pRoom == ch->in_room)
    {
        if (arg[0] == '\0')
        {
            if (ch->fighting)
                victim = ch->fighting;
            else
            {
                send_to_char("Shoot a firebolt at whom?\n\r", ch);
                return FALSE;
            }
        }
        else if ((victim = get_char_room(ch,arg)) == NULL)
        {
            send_to_char("You don't see them here.\n\r",ch);
            return FALSE;
        }
    }
    else
    {
        victim = get_char_room(ch, pRoom, arg);
        if (!victim)
        {
            send_to_char("You can't seem to see that person in that direction.\n\r",ch);
            return FALSE;
        }
    }

    if (is_safe_spell(ch, victim, FALSE))
        return FALSE;

    dam = dice(level, 5) + level/3;

    if (saves_spell(level, ch, victim, DAM_FIRE))
        dam /= 2;

    if (pRoom != ch->in_room)
        dam /= 2;

    /* 1/2 damage if they save, 1/2 damage again if it's one room away. */

    act("You point towards $N, and a bolt of fire streaks towards $M!",ch,NULL,victim,TO_CHAR);

    if (pRoom == ch->in_room)
    {
        act("$n points at $N, and a firebolt streaks towards $M!",ch,NULL,victim,TO_NOTVICT);
        act("$n points, and a bolt of fire streaks towards you!",ch,NULL,victim,TO_VICT);
    }
    else
    {
        char buf[MAX_STRING_LENGTH];

        sprintf(buf, "$n points, and a bolt of fire streaks %s!",
			( dir == 0 ? "northwards"
                        : dir == 1 ? "eastwards"
                        : dir == 2 ? "southwards"
                        : dir == 3 ? "westwards"
                        : dir == 4 ? "upwards"
                        : dir == 5 ? "down below you" : "away"));
        act(buf, ch, NULL, NULL, TO_ROOM);
        act("A bolt of fire flies into the room, streaking towards $n!", victim, NULL, NULL, TO_ROOM);
        act("A bolt of fire flies into the room, streaking towards you!", victim, NULL, NULL, TO_CHAR);
    }

    if ((pRoom != ch->in_room) && IS_NPC(victim) && ((IS_SET(victim->act, ACT_SENTINEL) || IS_SET(victim->act, ACT_NOTRACK))) && (victim->hit < victim->max_hit))
        dam = 0;

    if (pRoom != ch->in_room && check_defensiveroll(victim))
    {
    	send_to_char("You roll as the bolt of fire strikes you, lessening its affect.\n\r",victim);
	    dam /= 2;
    }
    damage_old( ch, victim, dam, sn,DAM_FIRE,TRUE);

    return TRUE;
}

bool spell_firebrand( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    OBJ_DATA *vObj = (OBJ_DATA *) vo;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if (!(vObj->item_type == ITEM_WEAPON || vObj->item_type == ITEM_ARROW))
    {
        send_to_char("Only weapons may be given a firebrand.\n\r", ch);
        return FALSE;
    }

    if (obj_is_affected(vObj, gsn_firebrand) || obj_is_affected(vObj, gsn_soulbrand) || (vObj->value[3] == attack_lookup("flbite")))
    {
        send_to_char("That weapon is already surrounded in flames.\n\r", ch);
        return FALSE;
    }

    if (IS_SET(vObj->extra_flags[0], ITEM_BLESS)
     || IS_SET(vObj->extra_flags[0], ITEM_ANTI_EVIL)
     || obj_is_affected(vObj, gsn_aura))
    {
        act("The aura of purity surrounding $p prevents your attempt to firebrand it.", ch, vObj, NULL, TO_CHAR);
        return FALSE;
    }

    if (vObj->value[3] == DAM_COLD)
    {
        send_to_char("You cannot firebrand a cold-based weapon.\n\r", ch);
        return FALSE;
    }
    
    if (vObj->value[3] == DAM_FIRE)
    {
	send_to_char("That weapon is already on fire.\n\r", ch);
	return FALSE;
    }

    af.where     = TO_OBJECT;
    af.type      = sn;
    af.level     = level;
    af.modifier  = 0 /*vObj->value[3]*/;
    af.location  = 0;
    af.duration  = level/3;
    af.bitvector = 0;
    af.point	 = NULL /*(void *) vObj->obj_str*/;
    affect_to_obj(vObj, &af);
// brazen: Ticket #126: if the affect of a brand spell is removed improperly, the
// type can be permanently changed. These spells are now handled in damage_from_obj
//    vObj->value[3] = DAM_FIRE;
//    vObj->obj_str = str_dup("flaming strike");

    act("$p is surrounded in flames.", ch, vObj, NULL, TO_CHAR);
    act("$n chants, and $p is surrounded in flames.", ch, vObj, NULL, TO_ROOM);

    return TRUE;
}

bool spell_firestorm( int sn, int level,CHAR_DATA *ch,void *vo,int target)
{
    char buf[MAX_STRING_LENGTH];
    CHAR_DATA *vch;
    CHAR_DATA *vch_next;

    // Check for cooldown
    if (is_affected(ch, sn))
    {
        send_to_char("You are not yet ready to engulf this place in flame.\n", ch);
        return false;
    }

    // Apply cooldown
    AFFECT_DATA af = {0};
    af.where    = TO_AFFECTS;
    af.type     = sn;
    af.level    = level;
    af.duration = 2;
    affect_to_char(ch, &af);

    int baseDam = 50 + ((level - 1) * 3);

    send_to_char( "Your firestorm rages through the room!\n\r", ch );
    act( "$n conjures a storm of fire to eradicate the room!", ch, NULL, NULL, TO_ROOM );

    // Prepare ignite effect for use in loop
    AFFECT_DATA iaf = {0};
    iaf.where   = TO_AFFECTS;
    iaf.type    = gsn_ignite;
    iaf.level   = level;

    // Hit people in room
    for ( vch = char_list; vch != NULL; vch = vch_next )
    {
        vch_next        = vch->next;
        if ( vch->in_room == NULL )
            continue;

        if ((vch->in_room == ch->in_room) && (ch!=vch) && !(is_same_group(ch, vch)) && !is_safe_spell(ch, vch, TRUE))
        {
            if ((vch!=ch) && (!IS_IMMORTAL(vch) || (IS_IMMORTAL(vch) && can_see(ch,vch))))
            {
                if ((ch->fighting != vch) && !IS_NPC(vch))
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
                int dam = number_range( baseDam / 2, baseDam * 2 );
                if (saves_spell( level, ch, vch, DAM_FIRE))
                    damage_old(ch, vch, dam / 2, sn, DAM_FIRE, TRUE);
                else
                {
                    damage_old(ch, vch, dam / 2, sn, DAM_FIRE, TRUE);

                    act("The engulfing heat catches you ablaze!", vch, NULL, NULL, TO_CHAR);
                    act("The engulfing heat catches $n ablaze!", vch, NULL, NULL, TO_ROOM);
                    iaf.duration = number_range(2, 6);
                    affect_to_char(vch, &iaf);
                }
            }
            continue;
        }
    }

    checkMeltIcyPrisons(ch->in_room);
    return TRUE;
}

bool spell_flameshield(int sn,int level,CHAR_DATA *ch,void *vo, int target)
{
    CHAR_DATA *victim = ch;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if ( is_affected(victim, sn))
    {
        send_to_char("You are already guarded by a shield of flame.\n\r", ch);
        return FALSE;
    }


        act("Flames form and begin to dance around $n in a sphere.", ch, NULL, NULL, TO_ROOM);
        act("Flames form and begin to dance around you in a sphere.", ch, NULL,
NULL, TO_CHAR);

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


bool spell_flamestrike(int sn, int level, CHAR_DATA *ch, void *vo,int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;

    // Do basic flamestrike
    std::vector<DamageInfo> damage(1);
    damage[0].type = DAM_FIRE;
    damage[0].amount = dice(8 + level / 2, 8);
    if (saves_spell(level, ch, victim, DAM_FIRE))
        damage[0].amount /= 2;

    // Check for holy flame
    if (number_percent() <= get_skill(ch, gsn_holyflame))
    {
        check_improve(ch, victim, gsn_holyflame, true, 4);
        damage.push_back(DamageInfo(10 + dice(2, level / 2), DAM_HOLY));
        if (saves_spell(level, ch, victim, DAM_HOLY))
            damage[damage.size() - 1].amount /= 2;
    }
    else
        check_improve(ch, victim, gsn_holyflame, false, 4);

    // Check for incendiary spark
    if (number_percent() <= get_skill(ch, gsn_incendiaryspark))
    {
        check_improve(ch, victim, gsn_incendiaryspark, true, 4);
        damage.push_back(DamageInfo(dice(2, level / 2), DAM_LIGHTNING));
        if (saves_spell(level, ch, victim, DAM_LIGHTNING))
            damage[damage.size() - 1].amount /= 2;
    }
    else
        check_improve(ch, victim, gsn_incendiaryspark, false, 4);

    damage_new(ch, victim, damage, sn, true);

    // Check for death (by simple presence in the room)
    if (victim->in_room != ch->in_room)
        return true;

    // Check for pyrotechnic artistry
    int skill(get_skill(ch, gsn_pyrotechnicartistry));
    if (skill <= 0)
        return true;

    if (number_percent() >= (skill / 2))
        return true;

    // Succeeded, make some pyrotechnics
    act("$n's flamestrike explodes in a scintillating array of colors and sounds!", ch, NULL, NULL, TO_ROOM);
    act("Your flamestrike explodes in a scintillating array of colors and sounds!", ch, NULL, NULL, TO_CHAR);
    int burstCount(number_range(1, 1 + (level / 17) + ((skill - 80) / 10)));
    for (int i(0); i < burstCount; ++i)
    {
        const char * mess("shower of sparks");
        switch (number_range(1, 7))
        {
            case 1: mess = "shower of blue sparks"; break;
            case 2: mess = "shower of red sparks"; break;
            case 3: mess = "shower of green sparks"; break;
            case 4: mess = "shower of violet sparks"; break;
            case 5: mess = "shower of white sparks"; break;
            case 6: mess = "shower of yellow sparks"; break;
            case 7: mess = "shower of orange sparks"; break;
        }
        damage_new(ch, victim, number_range(1, 1 + (level / 12)), gsn_pyrotechnicartistry, DAM_FIRE, true, const_cast<char*>(mess));
    }
    
    // Check for lost attacks
    if (!saves_spell(level, ch, victim, DAM_ILLUSION))
    {
        // Lost one attack, check for one more if the skill is high
        ++victim->lost_att;
        if (skill >= 90 && !saves_spell(level, ch, victim, DAM_ILLUSION))
            ++victim->lost_att;

        act("$N appears distracted by the coruscating lights!", ch, NULL, victim, TO_CHAR);
        act("$N appears distracted by the coruscating lights!", ch, NULL, victim, TO_NOTVICT);
        act("You are distracted by the coruscating lights!", ch, NULL, victim, TO_VICT);
    }

    check_improve(ch, victim, gsn_pyrotechnicartistry, true, 4);
    return true;
}

bool spell_flametongue( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    if (victim == ch)
        {
        act("$n concentrates, glowing momentarily.", ch, NULL, NULL, TO_ROOM);
        act("You concentrate, glowing momentarily.", ch, NULL, NULL, TO_CHAR);
        }
    else
        {
        act("$n lifts a hand towards $N, who is bathed in an unearthly fire!", ch, NULL, victim, TO_NOTVICT);
        act("You lift a hand towards $N, who is bathed in an unearthly fire!", ch, NULL, victim, TO_CHAR);
        act("$n lifts a hand towards you, and you are bathed in an unearthly fire!", ch, NULL, victim, TO_VICT);
        }

    if (check_dispel(level,victim,gsn_waterbreathing))
    {
        act("$n no longer breathes underwater.",victim,NULL,NULL,TO_ROOM);
        act("You are no longer able to breathe underwater.", victim, NULL, NULL, TO_CHAR);
    }
    if (check_dispel(level,victim,gsn_frostbite))
    {
        act("$n stops shivering with frostbite.",victim,NULL,NULL,TO_ROOM);
        act("You stop shivering with frostbite.", victim, NULL, NULL, TO_CHAR);
    }
    if (check_dispel(level,victim,gsn_sanctuary))
    {
        act("The white aura around $n fades away.",victim,NULL,NULL,TO_ROOM);
        act("Your white aura disappears.", victim, NULL, NULL, TO_CHAR);
    }
    if (check_dispel(level,victim,gsn_protectionfromfire))
    {
        act("$n's protection from fire vanishes.",victim,NULL,NULL,TO_ROOM);
        act("Your protection from fire vanishes.", victim, NULL, NULL, TO_CHAR);    }
    if (check_dispel(level,victim,gsn_protectionfrompoison))
    {
        act("$n's protection from poison vanishes.",victim,NULL,NULL,TO_ROOM);
        act("Your protection from poison vanishes.", victim, NULL, NULL, TO_CHAR);
    }

    return TRUE;
}



bool spell_flare( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    AFFECT_DATA * paf(get_affect(ch, sn));
    if (paf != NULL)
    {
        if (paf->modifier == APPLY_NONE) send_to_char("You are not ready to imbue yourself with more flame just yet.\n", ch);
        else send_to_char("You are already possessed of an inner fire.\n", ch);
        return false;
    }

    AFFECT_DATA af = {0};
    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = level / 3;
    af.modifier  = level / 3;;
    af.location  = APPLY_HIT;
    affect_to_char( ch, &af );

    act("A sphere of fire forms around $n and closes quickly, disappearing inside $m.", ch, NULL, NULL, TO_ROOM);
    act("A sphere of fire forms around you and closes quickly, disappearing inside you.", ch, NULL, NULL, TO_CHAR);
    return true;
}

bool spell_frenzy(int sn, int level, CHAR_DATA *ch, void *vo,int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if (is_affected(victim,sn) || IS_AFFECTED(victim,AFF_BERSERK))
    {
        if (victim == ch)
          send_to_char("You are already in a frenzy.\n\r",ch);
        else
          act("$N is already in a frenzy.",ch,NULL,victim,TO_CHAR);
        return FALSE;
    }

    if (is_affected(victim,skill_lookup("calm")))
    {
        if (victim == ch)
          send_to_char("Why don't you just relax for a while?\n\r",ch);
        else
          act("$N doesn't look like $e wants to fight anymore.", ch,NULL,victim,TO_CHAR);
        return false;
    }

    if (is_affected(victim, gsn_heartofstone))
    {
        if (victim == ch) send_to_char("Your stony heart cannot be stirred up into such passions.\n", ch);
        else act("Your magics fail to stir $N's passions.", ch, NULL, victim, TO_CHAR);
        return false;
    }

    // Effect * 1.5 for WreathOfFlame
    int multiplier = (getPyreInfoEffect(ch, PyreInfo::Effect_WreathOfFlame) == NULL ? 2 : 3);

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = level / 3;
    af.modifier  = (level * multiplier) / 10;
    af.bitvector = 0;

    af.location  = APPLY_HITROLL;
    affect_to_char(victim,&af);

    af.location  = APPLY_DAMROLL;
    affect_to_char(victim,&af);

    af.modifier  = (10 * level * multiplier) / 24;
    af.location  = APPLY_AC;
    affect_to_char(victim,&af);

    send_to_char("You are filled with rage!\n\r",victim);
    
// brazen: Ticket #344: Eyeless beings have no eyes for wild looks
    if(IS_SET(victim->parts,PART_EYE))
        act("$n gets a wild look in $s eyes!",victim,NULL,NULL,TO_ROOM);
    else
        act("$n gets a wild look about $m!",victim,NULL,NULL,TO_ROOM);

    return TRUE;
}

bool spell_fury(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if (is_affected(ch, sn))
    {
	send_to_char("You are already filled with the fury of the inferno.\n\r", ch);
	return FALSE;
    }

    af.where	 = TO_AFFECTS;
    af.type	 = sn;
    af.level	 = level;
    af.duration  = level/10;
    af.location  = APPLY_DAMROLL;
    af.modifier  = level / 2;
    af.bitvector = 0;
    affect_to_char(ch, &af);

    send_to_char("You are filled with the fury of the inferno!\n\r", ch);
    act("$n's veins bulge as a wild look overtakes $m!", ch, NULL, NULL, TO_ROOM);

    return TRUE;
}


bool spell_heat_metal( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    OBJ_DATA *obj;

    act("You send a blast of heat at $N's gear.", ch, NULL, victim, TO_CHAR);

    for (obj = victim->carrying; obj != NULL; obj = obj->next_content)
    {
	if (ch->in_room != victim->in_room || IS_OAFFECTED(victim, AFF_GHOST))
	    continue;

//        if ((obj->wear_loc > 0) && material_table[obj->material].metal)

	if (obj->worn_on && material_table[obj->material].metal)
        {
            act("$p burns $n!", victim, obj, NULL, TO_ROOM);
            act("$p burns you!", victim, obj, NULL, TO_CHAR);
            damage_old(victim,victim,number_range(ch->level/4, ch->level/2),sn,DAM_FIRE,TRUE);
        }
    }

    return TRUE;
}

bool spell_immolation( int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA * victim(NULL);
    if (target_name[0] == '\0') victim = ch;
    else victim = get_char_room(ch, target_name);
    
    if (victim == NULL)
    {
        // Check for object 
        OBJ_DATA * obj(get_obj_here(ch, target_name));
        if (obj == NULL)
        {
            send_to_char("There is nothing and no one here by that name.\n", ch);
            return false;
        }

        // Get the object and check for type light
        if (obj->item_type != ITEM_LIGHT)
        {
            send_to_char("You cannot imbue that with immolation.\n", ch);
            return false;
        }

        // Check for permanent light
        if (obj->value[2] < 0)
        {
            act("$p already glows eternally.", ch, obj, NULL, TO_CHAR);
            return false;
        }

        // Check for cooldown on this object
        if (obj_is_affected(obj, sn))
        {
            act("$p is already immolated.", ch, obj, NULL, TO_CHAR);
            return false;
        }

        act("$n gestures at $p, and it seems to glow brighter for a moment.", ch, obj, NULL, TO_ROOM);
        act("You gesture at $p, and it seems to glow brighter for a moment.", ch, obj, NULL, TO_CHAR);
        obj->value[2] += level;

        // Prevent reimmolation
        AFFECT_DATA af = {0};
        af.where        = TO_OBJECT;
        af.type         = sn;
        af.level        = level;
        af.duration     = -1;
        affect_to_obj(obj, &af);
        return true;
    }
    
    AFFECT_DATA af;
    af.valid = TRUE;
    af.point = NULL;

    if ( is_affected( victim, sn ) )
    {
        if (victim == ch)
          send_to_char("You are already enflamed.\n\r",ch);
        else
          act("$N already burns with immolation.",ch,NULL,victim,TO_CHAR);
        return FALSE;
    }

    // Check for grip of elanthemir (aka breath of elanthemir)
    if (is_affected(victim, gsn_breathofelanthemir))
    {
        if (ch == victim) send_to_char("You are chilled to the core with a power far too great for your immolating fires.\n", ch);
        else act("$N is chilled to the core with a power far too great for your immolating fires.", ch, NULL, victim, TO_CHAR);
        return false;
    }

    // Effect * 1.5 for WreathOfFlame
    int multiplier = (getPyreInfoEffect(ch, PyreInfo::Effect_WreathOfFlame) == NULL ? 2 : 3);
    int bonus = (is_familiar_present(*ch, gsn_callfox, OBJ_VNUM_FAMILIAR_FOX) ? 3 : 0);

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = 6+level;
    af.location  = APPLY_DAMROLL;
    af.modifier  = UMAX(1, (level * multiplier) / 16) + bonus;
    af.bitvector = 0;
    affect_to_char( victim, &af );

    af.location  = APPLY_SAVING_SPELL;
    af.modifier  = UMIN(-1, ((0 - level) * multiplier) / 16) - bonus;
    affect_to_char( victim, &af );
    send_to_char( "You feel enflamed.\n\r", victim );
    if ( ch != victim )
        act("You grant immolation to $N.",ch,NULL,victim,TO_CHAR);
    return TRUE;
}



bool spell_implosion( int sn, int level,CHAR_DATA *ch,void *vo,int target)
{
    char buf[MAX_INPUT_LENGTH];
    CHAR_DATA *vch;
    CHAR_DATA *vch_next;
    int dam;

    if (!ch->in_room)
	return FALSE;

    dam = number_range(1, 900);

    act("$n closes $s eyes, drawing in energy.", ch, NULL, NULL, TO_ROOM);
    act("You close your eyes, drawing in energy.", ch, NULL, NULL, TO_CHAR);
    if (dam > 50)
    {
	act("An implosion centers on $n, then erupts back out, incinerating everything!", ch, NULL, NULL, TO_ROOM);
        act("An implosion centers on you, then erupts back out, incinerating everything!", ch, NULL, NULL, TO_CHAR);
    }
    else
    {
        act("An implosion centers on $n, but the conflicting forces fizzle, causing a small blast of flame.", ch, NULL, NULL, TO_ROOM);
        act("An implosion centers on you, but the conflicting forces fizzle, causing a small blast of flame.", ch, NULL, NULL, TO_CHAR);
    }

    for (vch = ch->in_room->people; vch; vch = vch_next )
    {
        vch_next        = vch->next_in_room;

        if ((vch->in_room == ch->in_room)
         && (ch != vch)
         && !is_safe_spell(ch,vch,TRUE))
        {
            if ((vch->fighting != ch) && !IS_NPC(ch))
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

            damage_old( ch, vch, dam, sn,DAM_FIRE,TRUE);
        }
    }

    damage_old( ch, ch, dam, sn,DAM_FIRE,TRUE);
    checkMeltIcyPrisons(ch->in_room);
    return TRUE;
}

bool spell_incineration( int sn, int level, CHAR_DATA *ch, void *vo, int target){
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
    int dam;

    dam = dice( level, 4 );
    if ( saves_spell( level, ch, victim, DAM_FIRE ) )
        dam /= 2;
    act("$n raises $s hand, and fire sprays from $s outstretched arm, engulfing $N!", ch, NULL, victim, TO_NOTVICT);
    act("You raise your hand, and fire sprays from your outstretched arm, engulfing $N!", ch, NULL, victim, TO_CHAR);
    act("$n raises $s hand, and fire sprays from $s outstretched arm, engulfing you!", ch, NULL, victim, TO_VICT);
    damage_old( ch, victim, dam, sn,DAM_FIRE,TRUE);

    update_pos(victim);
    if (IS_OAFFECTED(victim, AFF_GHOST))
    {
        af.where     = TO_AFFECTS;
        af.type      = sn;
        af.level     = level;
        af.duration  = 30;
        af.location  = APPLY_NONE;
        af.modifier  = 0;
        af.bitvector = 0;
        affect_to_char( victim, &af );
    }

    return TRUE;
}

bool spell_infravision( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if ( IS_AFFECTED(victim, AFF_INFRARED) )
    {
        if (victim == ch)
          send_to_char("You can already see in the dark.\n\r",ch);
        else
          act("$N already has infravision.",ch,NULL,victim,TO_CHAR);
        return FALSE;
    }
    act( "$n's eyes glow red.", victim, NULL, NULL, TO_ROOM );

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = 2 * level;
    af.location  = APPLY_NONE;
    af.modifier  = 0;
    af.bitvector = AFF_INFRARED;
    affect_to_char( victim, &af );
    send_to_char( "Your eyes glow red.\n\r", victim );
    return TRUE;
}



bool spell_livingflame( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
    AFFECT_DATA af;
    af.point = NULL;
    CHAR_DATA *victim = (CHAR_DATA *)vo;

    if ( is_affected( victim, sn ) || IS_AFFECTED(victim,AFF_HASTE)
    ||   IS_SET(victim->off_flags,OFF_FAST))
    {
        if (ch==victim)
          send_to_char("You can't move any faster!\n\r",ch);
        else
          send_to_char("They can't move any faster!\n\r",ch);
        return FALSE;
    }

    // Handle WreathOfFlame
    int multiplier = 2;
    PyreInfo * pyreInfo(getPyreInfoEffect(ch, PyreInfo::Effect_WreathOfFlame));
    if (pyreInfo != NULL)
    {
        multiplier = 3;
        if (pyreInfo->isEffectGreater())
            af.point = reinterpret_cast<void*>(0x1);
    }

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = level/3;
    af.location  = APPLY_DEX;
    af.modifier  = 1 + (level >= 18) + (level >= 25) + (level >= 32);
    af.bitvector = AFF_HASTE;
    affect_to_char( victim, &af );

    af.location = APPLY_HITROLL;
    af.modifier = (level * multiplier) / 20;
    af.bitvector = 0;
    affect_to_char( victim, &af );

    af.location = APPLY_DAMROLL;
    affect_to_char( victim, &af );

    af.location = APPLY_HIT;
    af.modifier = (level * multiplier) / 2;
    affect_to_char(victim, &af);
    victim->hit += level;

    if (ch == victim)
        {
        send_to_char( "You are surrounded with a fiery glow and feel envigorated.\n\r", ch );
        act("$n shines briefly with a fiery glow.",ch,NULL,NULL,TO_ROOM);
        }
    else
        {
        act("You are surrounded with a fiery glow and feel envigorated.", victim, NULL, NULL, TO_CHAR);
        act("$n shines briefly with a fiery glow.",victim,NULL,NULL,TO_ROOM);
        }
    return TRUE;
}

bool spell_melt_metal( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    OBJ_DATA *obj, *obj_next;
    int chance;
    bool melted = FALSE;

    act("You send a blast of searing heat at $N.", ch, NULL, victim, TO_CHAR);
    act("$n sends a blast of searing heat at $N.", ch, NULL, victim, TO_NOTVICT);
    act("$n sends a blast of searing heat at you.", ch, NULL, victim, TO_VICT);

    if (saves_spell(level, ch, victim, DAM_FIRE))
    {
	send_to_char("You evade the searing blast.\n\r", victim);
	act("$n avoids the searing blast.", victim, NULL, NULL, TO_ROOM);
	return TRUE;
    }

    for (obj = victim->carrying; obj != NULL; obj = obj_next)
    {
	obj_next = obj->next_content;

	if (obj->worn_on && material_table[obj->material].metal && !IS_SET(obj->extra_flags[0], ITEM_NODESTROY) && !IS_OBJ_STAT(obj, ITEM_BURN_PROOF))
	{
	    damage(ch, victim, number_range(victim->level/12, victim->level/5), sn, DAM_FIRE, TRUE);
	    chance = UMAX(2, level - obj->level);
	    if (!melted && (number_percent() < chance))
	    {
		act("$p melts away under the intense heat.", ch, obj, victim, TO_CHAR);
		act("$p melts away under the intense heat.", ch, obj, victim, TO_VICTROOM);
		act("$p melts away under the intense heat.", ch, obj, victim, TO_VICT);
                damage_old(ch,victim,number_range(ch->level/2, ch->level),sn,DAM_FIRE,TRUE);
		extract_obj(obj);
		melted = TRUE;
	    }
        }
   }

   return TRUE;
}

bool spell_nova(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
    CHAR_DATA *victim = NULL;
    int length, dam, idam, edam;

    if (is_affected(ch, sn))
    {
	if (target_name[0] == '\0')
	{
	    if (ch->fighting == NULL)
	    {
	        send_to_char("Upon whom do you wish to unleash your stored nova?\n\r", ch);
	        return FALSE;
 	    }
	    else
		victim = ch->fighting;
	}
	else
	    if ((victim = get_char_room(ch, target_name)) == NULL)
	    {
	        send_to_char("You don't seem them here.\n\r", ch);
	        return FALSE;
	    }

	if (is_safe_spell(ch, victim, FALSE))
	    return FALSE;

	act("You thrust your hands towards $N, unleashing the power of the nova upon $M!", ch, NULL, victim, TO_CHAR);
	act("$n thrusts $s hands towards $N, unleashing the power of the nova upon $M!", ch, NULL, victim, TO_NOTVICT);
	act("$n thrusts $s hands towards you, and you are struck by the power of the nova!", ch, NULL, victim, TO_VICT);

	idam = dice(level, 5) + level/3;
	edam = (dice(level, 5) + level/3) * (get_modifier(ch->affected, gsn_nova)+2);
	dam = idam + ((edam - idam) * 6 / 10);
	affect_strip(ch, gsn_nova);

	damage_old(ch, victim, dam, gsn_nova, DAM_FIRE, TRUE);
	return TRUE;
    }
    else
	if (ch->fighting != NULL)
	{
	    send_to_char("You can't charge a nova while fighting!\n\r",ch);
	    return FALSE;
	}	

    if ((target_name[0] == '\0') || !is_number(target_name))
    {
	send_to_char("How many ticks do you wish to charge the nova?\n\r", ch);
	return FALSE;
    }

    length = atoi(target_name);

    if ((length < 1) || (length > 3))
    {
	send_to_char("Valid charging lengths are between 1 and 3.\n\r", ch);
	return FALSE;
    }

    send_to_char("You begin to concentrate intently, a ball of intense fire forming between your hands.\n\r", ch);
    act("$n begins to concentrate intently, a ball of intense fire forming between $s hands.", ch, NULL, NULL, TO_ROOM);

    af.where	 = TO_OAFFECTS;
    af.type	 = sn;
    af.level	 = level;
    af.duration  = length;
    af.modifier  = length;
    af.location  = APPLY_NONE;
    af.bitvector = AFF_NOVA_CHARGE;
    affect_to_char(ch, &af);

    return TRUE;
}
    
bool spell_parch( int sn, int level, CHAR_DATA *ch,void *vo,int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if (is_affected(victim, gsn_parch))
        {
        send_to_char("They are already parched.\n\r", ch);
        return FALSE;
        }

   if (saves_spell(level, ch, victim, DAM_FIRE))
    {
        if (ch != victim)
          send_to_char("You feel hot and dry momentarily, but it passes.\n\r",victim);
        send_to_char("Spell failed.\n\r", ch);
        return TRUE;
    }

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = level/4;
    af.location  = 0;
    af.modifier  = 0;
    af.bitvector = 0;
    affect_to_char(victim,&af);

    if (!IS_NPC(victim))
        victim->pcdata->condition[COND_THIRST] = 0;
   act("$n looks thirsty.", victim, NULL, NULL, TO_ROOM);
   act("You feel very thirsty.", victim, NULL, NULL, TO_CHAR);
    return TRUE;
}


bool spell_phoenixfire( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
    AFFECT_DATA * check_af;

    if (IS_OAFFECTED(ch,AFF_GHOST))
    {
	send_to_char("It's too late.\n\r",ch);
	return FALSE;
    }

    if ( (check_af = get_affect( ch, sn )) != NULL )
    {
	if (check_af->modifier == 0)
	    send_to_char("You do not feel ready to call upon the power of the phoenix yet.\n\r", ch);
	else
	    send_to_char("You already are wrapped in the power of the phoenix.\n\r", ch);
        return FALSE;
    }
    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = level/9;
    af.modifier  = 1;
    af.location  = 0;
    af.bitvector = 0;
    affect_to_char( ch, &af );

    act("An image of a flaming phoenix is superimposed on $n, then fades away.", ch, NULL, NULL, TO_ROOM);
    act("You feel the power of the phoenix infuse you.", ch, NULL, NULL, TO_CHAR);

    return TRUE;
}

bool spell_pyrotechnics( int sn, int level,CHAR_DATA *ch,
        void *vo,int target)
{
    char buf[MAX_STRING_LENGTH];
    CHAR_DATA *vch;
    CHAR_DATA *vch_next;
    static const int dam_each[] =
    {
          5,
          5,   5,   5,   5,   5,          7,   7,   7,   7,   7,
         10,  10,  10,  10,  15,         17,  20,  22,  25,  27,
         30,  33,  35,  37,  40,         41,  42,  43,  44,  45,
         46,  47,  48,  49,  50,         51,  52,  53,  54,  55,
         56,  57,  58,  59,  60,         61,  62,  63,  64,  65
    };
    int dam;

    level       = UMIN(level, static_cast<int>(sizeof(dam_each)/sizeof(dam_each[0])) - 1);
    level       = UMAX(0, level);

    send_to_char( "Your pyrotechnic explosion engulfs the room!\n\r", ch);
    act( "$n conjures an explosion of pyrotechnics to engulf the room!",
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
           dam  = number_range( dam_each[level] / 3*2, dam_each[level] *3/2 );
                   if ( saves_spell( level, ch, vch, DAM_FIRE) )
                        dam /= 2;

                damage_old( ch, vch, saves_spell( level,ch, vch,DAM_FIRE)
                ? dam / 2 : dam, sn,DAM_FIRE,TRUE);
            }
            continue;
        }
    }

    checkMeltIcyPrisons(ch->in_room);
    return TRUE;
}


bool spell_rainoffire( int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
DESCRIPTOR_DATA *d;


        if (is_affected(ch, gsn_rainoffire))
        {
        send_to_char("You cannot summon the power to call another rain of fire yet.\n\r", ch);
        return FALSE;
        }

        if (area_is_affected(ch->in_room->area, gsn_rainoffire))
        {
        send_to_char("Fire is already raining down upon this area.\n\r", ch);
        return FALSE;
        }

        af.where        = TO_AREA;
        af.type = sn;
        af.level        = level;
        af.duration     = 4;
        af.location     = 0;
        af.modifier     = 0;
        af.bitvector    = 0;
        affect_to_area(ch->in_room->area, &af);

        af.duration     = 120;
        affect_to_char(ch, &af);

        for (d = descriptor_list; d != NULL; d = d->next)
        {
        if (d->connected == CON_PLAYING && d->character && d->character->in_room)
            if (d->character->in_room->area == ch->in_room->area)
                send_to_char("The sky glows red as fire begins to rain down from above!\n\r", d->character);
        }
    return TRUE;
}

bool spell_ringoffire( int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *vch;
    AFFECT_DATA af;
    int dam = 0;
af.valid = TRUE;
af.point = NULL;

    if (room_is_affected(ch->in_room, sn))
    {
	send_to_char("There is already a ring of fire here.\n\r", ch);
	return FALSE;
    }

    af.where	 = TO_ROOM;
    af.type	 = sn;
    af.level	 = level;
    af.location = APPLY_NONE;
    af.duration  = level/8;
    af.modifier  = 0;
    af.bitvector = 0;
    affect_to_room(ch->in_room, &af);

    act("A ring of fire radiates outward from you, encircling the room in flames!",
	ch, NULL, NULL, TO_CHAR);
    act("A ring of fire radiates outward from $n, encircling the room in flames!",
	ch, NULL, NULL, TO_ROOM);

    for (vch = ch->in_room->people; vch; vch=vch->next_in_room)
    {
	if (!IS_NPC(vch) && !IS_PK(ch,vch))
	    continue;
	dam = number_range(level/2,level*1.5);
	if (saves_spell(level,ch, vch,DAM_FIRE))
	    dam /= 2;
        damage_old( ch, vch, dam, sn, DAM_FIRE, TRUE);
    }

    return TRUE;
}

bool checkDoorFireRune(CHAR_DATA * ch, int door)
{
    EXIT_DATA * pexit(ch->in_room->exit[door]);

    // Check for the bit
    if (pexit == NULL || !IS_SET(pexit->exit_info, EX_RUNEOFFIRE) || !IS_SET(pexit->exit_info, EX_CLOSED))
        return false;

    // Get the other side, removing fire rune bits along the way
    REMOVE_BIT(pexit->exit_info, EX_RUNEOFFIRE);
    ROOM_INDEX_DATA * otherRoom(pexit->u1.to_room);
    EXIT_DATA * otherExit(NULL);
    if (otherRoom != NULL && otherRoom->exit[rev_dir[door]] != NULL && otherRoom->exit[rev_dir[door]]->u1.to_room == ch->in_room)
    {
        otherExit = otherRoom->exit[rev_dir[door]];
        REMOVE_BIT(otherExit->exit_info, EX_RUNEOFFIRE);
        if (otherExit->key < 0 || IS_SET(otherExit->exit_info, EX_PICKPROOF))
           otherExit = NULL;
    }

    // Get the spell level and clean up the room affects
    int level(ch->level);
    AFFECT_DATA * aff(get_room_affect_with_modifier(otherRoom, gsn_runeoffire, door));
    if (aff != NULL)
    {
        level = aff->level;
        affect_remove_room(otherRoom, aff);
    }
    
    aff = get_room_affect_with_modifier(ch->in_room, gsn_runeoffire, door);
    if (aff != NULL)
    {
        level = aff->level;
        affect_remove_room(ch->in_room, aff);
    }

    // Rune present, it will explode, but the door may not open
    if (pexit->key < 0 || IS_SET(pexit->exit_info, EX_PICKPROOF))
    {
        act("As you try fruitlessly to breach the $d, the rune of fire on it blazes brightly into existence, then explodes!", ch, NULL, pexit->keyword, TO_CHAR);
        act("As $n tries fruitlessly to breach the $d, the rune of fire on it blazes brightly into existence, then explodes!", ch, NULL, pexit->keyword, TO_ROOM);

        if (otherExit != NULL && otherRoom->people != NULL)
        {
            act("You hear a sudden explosion from the other side of the $d!", otherRoom->people, NULL, otherExit->keyword, TO_CHAR);
            act("You hear a sudden explosion from the other side of the $d!", otherRoom->people, NULL, otherExit->keyword, TO_ROOM);
        }
    }
    else
    {
        // Breach this door
        act("As you breach the $d, the rune of fire on it blazes brightly into existence, then explodes! The $d blows open from the blast!", ch, NULL, pexit->keyword, TO_CHAR);
        act("As $n breaches the $d, the rune of fire on it blazes brightly into existence, then explodes! The $d blows open from the blast!", ch, NULL, pexit->keyword, TO_ROOM);
        REMOVE_BIT(pexit->exit_info, EX_RUNEOFEARTH);
        REMOVE_BIT(pexit->exit_info, EX_LOCKED);
        REMOVE_BIT(pexit->exit_info, EX_CLOSED);

        // Breach the other side if appropriate
        if (otherExit != NULL)
        {
            if (otherRoom->people != NULL)
            {
                act("You hear a sudden explosion, and the $d suddenly blows open in a burst of heat and flame!", otherRoom->people, NULL, otherExit->keyword, TO_CHAR);
                act("You hear a sudden explosion, and the $d suddenly blows open in a burst of heat and flame!", otherRoom->people, NULL, otherExit->keyword, TO_ROOM);
            }

            REMOVE_BIT(otherExit->exit_info, EX_RUNEOFEARTH);
            REMOVE_BIT(otherExit->exit_info, EX_LOCKED);
            REMOVE_BIT(otherExit->exit_info, EX_CLOSED);
        }
    }

    // Damage the breacher
    int dam(dice(4, level));
    if (saves_spell(level, NULL, ch, DAM_FIRE))
        dam /= 2;

    damage(ch, ch, dam, gsn_runeoffire, DAM_FIRE, true);
    return true;
}

bool spell_runeoffire( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
OBJ_DATA *obj = NULL;

        if (!ch->in_room)
                return FALSE;

        if ((obj = get_obj_list(ch, target_name, ch->in_room->contents)) == NULL)
        {
            // No object, so check for a door
            int door(find_door(ch, target_name));
            if (door < 0)
            {
                send_to_char("There is nothing by that name here on which to scribe the rune.\n", ch);
                return false;
            }

            // Get the exit, check for closed
            EXIT_DATA * pexit(ch->in_room->exit[door]);
            if (!IS_SET(pexit->exit_info, EX_CLOSED))
            { 
               send_to_char("The door is not closed.\n", ch); 
               return false; 
            }

            // Check for already existing
            if (IS_SET(pexit->exit_info, EX_RUNEOFFIRE))
            {
                send_to_char("A rune of fire has already been burned there.\n", ch);
                return false;
            }

            // Prepare the cooldown
            af.where     = TO_ROOM_AFF;
            af.type      = sn;
            af.level     = level;
            af.duration  = level / 10;
            af.location  = 0;
            af.modifier  = door;
            af.bitvector = 0;
            affect_to_room(ch->in_room, &af);

            // Set the rune bit, check for the opposite direction to set the bit as well
            SET_BIT(pexit->exit_info, EX_RUNEOFFIRE);
            ROOM_INDEX_DATA * otherRoom(pexit->u1.to_room);
            if (otherRoom != NULL)
            {
                // Other room exists; if the other door exists and points here, set the bit
                EXIT_DATA * otherExit(otherRoom->exit[rev_dir[door]]);
                if (otherExit != NULL && otherExit->u1.to_room == ch->in_room)
                {
                    SET_BIT(otherExit->exit_info, EX_RUNEOFFIRE);
                    af.modifier = rev_dir[door];
                    affect_to_room(otherRoom, &af);
                }
            }
            
            act("You carefully trace your finger over the $d, scribing a burning symbol which swiftly fades.", ch, NULL, pexit->keyword, TO_CHAR);
            act("$n carefully traces $s finger over the $d, scribing a burning symbol which swiftly fades.", ch, NULL, pexit->keyword, TO_ROOM);
            return true;
        }

        if (obj_is_affected(obj, gsn_runeoffire))
        {
        send_to_char("That object already has a rune of fire drawn upon it!\n\r", ch);
        return FALSE;
        }

        act("$n carefully gestures, and a line of fire traces a symbol on $p, which quickly fades.", ch, obj, NULL, TO_ROOM);
        act("You carefully gesture, and a line of fire traces a symbol on $p, which quickly fades.", ch, obj, NULL, TO_CHAR);

        af.where        = TO_OBJECT;
        af.type         = sn;
        af.level        = level;
        af.duration     = level;
        af.location     = 0;
        af.modifier     = 0;
        af.bitvector    = 0;
        affect_to_obj(obj,&af);
    return TRUE;
}

bool spell_scorch( int sn, int level, CHAR_DATA *ch,void *vo,int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    static const int dam_each[] =
    {
         0,
         4,  4,  5,  5,  6,      7,  7,  7,  7,  7,
         8,  8,  8,  8,  8,      9,  9,  9,  9,  9,
        10, 10, 10, 10, 10,     11, 11, 11, 11, 11,
        12, 12, 12, 12, 12,     13, 13, 13, 13, 13,
        14, 14, 14, 14, 14,     15, 15, 15, 15, 15
    };
    int dam;

    level       = UMIN(level, static_cast<int>(sizeof(dam_each)/sizeof(dam_each[0])) - 1);
    level       = UMAX(0, level);
    dam         = number_range( dam_each[level] / 2, dam_each[level] * 2 );
    if ( saves_spell( level, ch, victim,DAM_FIRE) )
        dam /= 2;
    act("$n summons a quick jet of flame to scorch $N.", ch, NULL, victim,
        TO_ROOM);
    damage_old( ch, victim, dam, sn, DAM_FIRE ,TRUE);
    return TRUE;
}

bool spell_smoke( int sn, int level,CHAR_DATA *ch,void *vo,int target)
{
    char buf[MAX_STRING_LENGTH];
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
    CHAR_DATA *vch;
    CHAR_DATA *vch_next;

    // Check for cooldown
    for (AFFECT_DATA * paf(get_affect(ch, gsn_smoke)); paf != NULL; paf = get_affect(ch, gsn_smoke, paf))
    {
        if (paf->bitvector == 0)
        {
            send_to_char("You cannot summon a cloud of smoke again yet.\n\r", ch);
            return false;
        }
    }

    int baseDam = 3 + (level - 4);

    send_to_char( "Your cloud of smoke engulfs the room!\n\r", ch );
    act( "$n fills the room with smoke!",
        ch, NULL, NULL, TO_ROOM );

    for ( vch = char_list; vch != NULL; vch = vch_next )
    {
        vch_next        = vch->next;
        if ( vch->in_room == NULL )
            continue;
        if ((vch->in_room == ch->in_room) && (ch!=vch) &&
                !is_same_group(ch,vch) && !is_safe_spell(ch,vch,TRUE))
        {
            if ((vch!=ch) && (!IS_IMMORTAL(vch) ||
               (IS_IMMORTAL(ch) && can_see(ch,vch))))
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

                int dam= number_range( baseDam / 2, baseDam * 2 );
                if ( saves_spell( level, ch, vch, DAM_DROWNING) ) dam /= 2;
                damage_old( ch, vch, dam, sn,DAM_DROWNING,TRUE);
            }
            continue;
        }
    }

    fillRoomWithSmoke(ch, ch->in_room, level - 3, 3);

           af.where     = TO_AFFECTS;
           af.type      = sn;
           af.level     = level;
           af.location  = 0;
           af.modifier  = 0;
           af.duration  = 3;
           af.bitvector = 0;
           affect_to_char(ch, &af);

    return TRUE;
}




bool spell_smolder( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if (is_affected(victim, gsn_smolder))
        {
        send_to_char("They are already smoldering.\n\r", ch);
        return FALSE;
        }

   if (saves_spell(level, ch, victim, DAM_FIRE))
    {
        if (ch != victim)
          send_to_char("You feel intensely hot and dry, but it passes.\n\r",victim);
        send_to_char("Spell failed.\n\r", ch);
        return TRUE;
    }

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = level/4;
    af.location  = 0;
    af.modifier  = 0;
    af.bitvector = 0;
    affect_to_char(victim,&af);

   act("$n looks very uncomfortable and hot.", victim, NULL, NULL, TO_ROOM);
   act("You feel very uncomfortable and hot.", victim, NULL, NULL, TO_CHAR);
    return TRUE;
}


bool spell_walloffire(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    char buf[MAX_STRING_LENGTH];
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
    int direction, i;
    EXIT_DATA *pexit;

    if (!ch->in_room)
	return FALSE;

    if (is_affected(ch, gsn_walloffire))
    {
        send_to_char("You have summoned a wall of fire too recently to call another.\n\r", ch);
        return FALSE;
    }

    if (ch->in_room->sector_type == SECT_UNDERWATER)
    {
        send_to_char("You can't create a wall of fire underwater.\n\r", ch);
        return FALSE;
    }

    if (ch->in_room->sector_type == SECT_WATER_SWIM || ch->in_room->sector_type == SECT_WATER_NOSWIM)
    {
	send_to_char("You cannot summon a wall of fire here.\n\r", ch);
	return FALSE;
    }

    if (target_name[0] == '\0')
    {
        //send_to_char("Create a wall of fire in which direction?\n\r", ch);
        //return FALSE;
	i = 0;
	do
	{
	 i++;
	 direction = number_range(1,6);
	}while(ch->in_room->exit[direction] == NULL && i < 30);
    }
    else
    {
	if (!str_prefix(target_name, "north")) direction = 0;
    	else if (!str_prefix(target_name, "east"))	direction = 1;
	else if (!str_prefix(target_name, "south")) direction = 2;
    	else if (!str_prefix(target_name, "west"))	direction = 3;
    	else if (!str_prefix(target_name, "up"))	direction = 4;
    	else if (!str_prefix(target_name, "down"))	direction = 5;
    	else
    	{
		send_to_char("That's not a valid direction.\n\r", ch);
	        return FALSE;
    	}
    }
    if ((pexit = ch->in_room->exit[direction]) == NULL)
    {
	send_to_char("There's no exit there to block!\n\r", ch);
        return FALSE;
    }

    if (IS_SET(ch->in_room->exit[direction]->exit_info, EX_WALLOFFIRE))
    {
	send_to_char("There's already a wall of fire blocking that!\n\r", ch);
        return FALSE;
    }

    SET_BIT(ch->in_room->exit[direction]->exit_info, EX_WALLOFFIRE);

    sprintf(buf, "A blast of heat and a roar of flame accompany the rise of a wall of fire %s.",
	((direction == 0) ? "to the north" : (direction == 1) ? "to the east" :
	 (direction == 2) ? "to the south" : (direction == 3) ? "to the west" :
	 (direction == 4) ? "above you" : "below you"));

    act(buf, ch, NULL, NULL, TO_ROOM);
    act(buf, ch, NULL, NULL, TO_CHAR);

    if (ch->in_room->exit[direction]->u1.to_room->exit[OPPOSITE(direction)]
     && ch->in_room->exit[direction]->u1.to_room->exit[OPPOSITE(direction)]->u1.to_room == ch->in_room
     && !IS_SET(ch->in_room->exit[direction]->u1.to_room->exit[OPPOSITE(direction)]->exit_info, EX_WALLOFFIRE))
    {
	SET_BIT(ch->in_room->exit[direction]->u1.to_room->exit[OPPOSITE(direction)]->exit_info, EX_WALLOFFIRE);

        if (ch->in_room->exit[direction]->u1.to_room->people)
        {
	    sprintf(buf, "A blast of heat and a roar of flame accompany the rise of a wall of fire %s.",
		((OPPOSITE(direction) == 0) ? "to the north" : (OPPOSITE(direction) == 1) ? "to the east" :
		 (OPPOSITE(direction) == 2) ? "to the south" : (OPPOSITE(direction) == 3) ? "to the west" :
		 (OPPOSITE(direction) == 4) ? "below you" : "above you"));

            act(buf, ch->in_room->exit[direction]->u1.to_room->people, NULL, NULL, TO_ROOM);
            act(buf, ch->in_room->exit[direction]->u1.to_room->people, NULL, NULL, TO_CHAR);
        }
    }

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = 24;
    af.location  = 0;
    af.modifier  = 0;
    af.bitvector = 0;
    affect_to_char( ch, &af );

    af.where	 = TO_ROOM_AFF;
    af.duration  = level / 3;
    af.modifier  = direction;
    affect_to_room(ch->in_room, &af);

    return TRUE;
}

bool spell_wardoffire(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if (is_affected(victim, sn))
    {
	if (victim == ch)
	    send_to_char("You are already protected by a ward of fire.\n\r", ch);
	else
	    act("$N is already protected by a ward of fire.", ch, NULL, victim, TO_CHAR);
	return FALSE;
    }

    if (victim->fighting)
    {
	act("$N must be standing still in order to inscribe a ward around them.", ch, NULL, victim, TO_CHAR);
	return FALSE;
    }

    af.where	 = TO_AFFECTS;
    af.type	 = sn;
    af.level	 = level;
    af.duration  = 10;
    af.modifier  = 0;
    af.location  = 0;
    af.bitvector = 0; 
    affect_to_char(victim, &af);

    if (victim == ch)
    {
	send_to_char("You carefully inscribe a ward of fire upon the ground.\n\r", ch);
	send_to_char("You stand within the rune, chanting softly, and it begins to glow a bright crimson.\n\r", ch);
        act("$n carefully inscribes a rune of fire upon the ground.", ch, NULL, NULL, TO_ROOM);
        act("$n stands within the rune, chanting softly, and it begins to glow a bright crimson.", ch, NULL, NULL, TO_ROOM);
    }
    else
    {
	act("You carefully inscribe a ward of fire upon the ground around $N.", ch, NULL, victim, TO_CHAR);
	send_to_char("You chant softly, and the ward of fire begins to glow a bright crimson.\n\r", ch);
	act("$n carefully inscribes a ward of fire on the ground around you.", ch, NULL, victim, TO_VICT);
	act("$n carefully inscribes a ward of fire on the ground around $N.", ch, NULL, victim, TO_NOTVICT);
	act("$n chants softly, and the ward of fire begins to glow a bright crimson.", ch, NULL, NULL, TO_ROOM);
    }
 
    return TRUE;
}

bool spell_weariness(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *vch, *vch_next;
    char buf[MAX_STRING_LENGTH];
    int movedam=0;
    AFFECT_DATA af;
    af.valid = TRUE;
    af.point = NULL;

    if (!ch->in_room)
	return FALSE;

    send_to_char("You summon a wave of wearying heat.\n\r", ch);

    for (vch = ch->in_room->people; vch; vch = vch_next)
    {
	vch_next = vch->next_in_room;

	if (!is_same_group(ch, vch)
	 && !is_safe_spell(ch, vch, TRUE)
	 && (!IS_NPC(vch) || !IS_AFFECTED(vch, AFF_WIZI)))
	{
	    if (!IS_NPC(vch) && !vch->fighting)
	    {
	        if (can_see(vch, ch))
		    sprintf(buf, "Help!  %s is attacking me!", PERS(ch, vch));
		else
		    sprintf(buf, "Help!  Someone is attacking me!");
		do_autoyell(vch, buf);
	    }
	    check_killer(ch, vch);

	    send_to_char("You feel weary as a wave of heat passes through you.\n\r", vch);

	    movedam = number_range(level,level*3);
            if ( saves_spell( level, ch, vch, DAM_FIRE ) )
	        movedam /= 2;
	    vch->move -= movedam;
	    damage(ch, vch, number_range(1,3), gsn_weariness, DAM_FIRE, FALSE);
	    if (!saves_spell(level, ch, vch,DAM_FIRE))
	    {
		send_to_char("The wave of heat drains your body!\n\r",vch);
		act("$n looks extremely weary.",vch,NULL,NULL,TO_ROOM);
		af.type = sn;
		af.where = TO_AFFECTS;
		af.bitvector = 0;
		af.duration = 6;
		af.modifier = -2;
		af.location = APPLY_STR;
		af.level = level;
		affect_to_char(vch,&af);
		af.location = APPLY_DEX;
		affect_to_char(vch,&af);
	    }
	    else
		act("$n looks more weary.",vch,NULL,NULL,TO_ROOM);
	}
    }

    return TRUE;
}

bool spell_wingsofflame(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    AFFECT_DATA af;
    af.valid = TRUE;
    af.point = NULL;

    if (IS_AFFECTED(ch, AFF_FLYING))
    {
        send_to_char("You are already airbone.\n\r",ch);
        return FALSE;
    }

    if (is_affected(ch, sn))
    {
        send_to_char("You already have wings of flame!\n", ch);
        return false;
    }

    if (!ch->in_room)
        return FALSE;

    if (area_is_affected(ch->in_room->area, gsn_gravitywell))
    {
        send_to_char("The pull to the ground is too much to allow flight.\n\r", ch);
        return FALSE;
    }

    if (is_affected(ch, gsn_earthbind))
    {
	send_to_char("Your binding to the earth is too powerful to fly.\n\r", ch);
	return FALSE;
    }

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = level/3;
    af.location  = APPLY_DEX;
    af.modifier  = 2;
    af.bitvector = AFF_FLYING;
    affect_to_char( ch, &af );

    act("$n unfurls wings of fire, and rises off the ground.", ch, NULL, NULL, TO_ROOM);
    act("You unfurl wings of fire, and rise off the ground.", ch, NULL, NULL, TO_CHAR);

    return TRUE;
}

bool spell_blazingspear( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    OBJ_DATA *spear;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if (is_affected(ch, sn))
    {
        send_to_char("You don't feel ready to create another blazing spear yet.\n\r",ch);
        return FALSE;
    }

    if ((spear = create_object(get_obj_index(OBJ_VNUM_FIRE_SPEAR), level)) == NULL)
    {
        bug("Spell: blazing spear.  Cannot load spear object.", 0);
        send_to_char("A problem has occured. Please contact the gods.\n\r", ch);
        return FALSE;
    }

    spear->level    = level;
    spear->timer    = 72;
    spear->value[0] = 3;
    spear->value[1] = 3;
    spear->value[2] = 8 + (level/10);
    spear->weight   = 120;

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = 48;
    af.location  = 0;
    af.modifier  = 0;
    af.bitvector = 0;
    affect_to_char(ch, &af);

    send_to_char("You raise your hand, and a blazing shaft of fire streams from your fist, coalescing into a spear.\n\r",ch);
    act("$n raises $s hand, and a blazing shaft of fire streams from $s fist, coalescing into a spear.", ch, NULL, NULL, TO_ROOM);

    obj_to_char(spear, ch);

    return TRUE;
}

bool spell_runeofembers( int sn, int level, CHAR_DATA *ch, void *vo, int target)
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

    if (is_affected(ch, sn))
    {
        send_to_char("You already have added a rune of embers recently.\n\r", ch);
        return FALSE;
    }

    if (obj_is_affected(weapon, sn))
    {
        send_to_char("That weapon already has a rune of embers.\n\r", ch);
        return FALSE;
    }

    if (weapon->item_type != ITEM_WEAPON)
    {
        send_to_char("You can only apply a rune of embers to a weapon.\n\r", ch);
        return FALSE;
    }

    af.where     = TO_OBJECT;
    af.type      = sn;
    af.level     = level;
    af.duration  = 24;
    af.location  = APPLY_MANA;
    af.modifier  = -50;
    af.bitvector = 0;
    affect_to_obj(weapon, &af);

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = 24;
    af.location  = 0;
    af.modifier  = 0;
    af.bitvector = 0;
    affect_to_char(ch, &af);

    act("$n utters a gutteral chant, branding $p with a rune of embers.",ch,weapon,NULL,TO_ROOM);
    act("You utter a gutteral chant, branding $p with a rune of embers.",ch,weapon,NULL,TO_CHAR);
    return TRUE;
}

void do_disruption(CHAR_DATA *ch, char *argument)
{
    CHAR_DATA *victim;
    OBJ_DATA *wield,*mantle;
    int chance, dam, td;
    char buf[MAX_STRING_LENGTH];

    if ((chance = get_skill(ch, gsn_disruption)) == 0)
    {
        send_to_char("Huh?\n\r", ch);
        return;
    }
    
    wield = get_eq_char(ch, WEAR_WIELD);

    if (!wield)
    {
	send_to_char("You must be wielding a weapon to disrupt your opponent.\n\r", ch);
	return;
    }

    if (argument[0] == '\0')
    {
	if (ch->fighting)
	    victim = ch->fighting;
	else
	{
	    send_to_char("Whom do you wish to disrupt?\n\r", ch);
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
	send_to_char("You can't disrupt yourself.\n\r",ch);
	return;
    }

    if (ch->mana < skill_table[gsn_disruption].min_mana)
    {
	send_to_char("You lack the energy to disrupt an opponent.\n\r", ch);
	return;
    }

    expend_mana(ch, skill_table[gsn_disruption].min_mana);

    if (is_safe(ch, victim))
	return;

    if (!check_reach(ch, victim, REACH_EASY, PULSE_VIOLENCE))
	return;

    WAIT_STATE(ch, UMAX(ch->wait, skill_table[gsn_disruption].beats));

    if (!IS_NPC(victim))
	if (ch->fighting == NULL || victim->fighting != ch)
	{
	    sprintf(buf, "Help! %s is attacking me!", PERS(ch, victim));
	    do_autoyell(victim, buf);
	}

    chance = (chance * 9) / 10;
    chance -= (get_skill(victim,gsn_dodge)/10);
    if (IS_AFFECTED(ch,AFF_HASTE) || IS_SET(ch->off_flags,OFF_FAST))
	chance += 10;
    if (IS_AFFECTED(victim,AFF_HASTE) || IS_SET(victim->off_flags,OFF_FAST))
	chance -= 10;
    if (IS_AFFECTED(ch,AFF_SLOW))
	chance -= 10;
    if (IS_AFFECTED(victim,AFF_SLOW))
	chance += 10;
    act("Chaotic energy surges through $p!",ch,wield,NULL,TO_CHAR);
    act("Chaotic energy surges through $p!",ch,wield,NULL,TO_ROOM);

    if (number_percent() > chance)
    {
	act("You swing at $N, but $E avoids the blow!", ch, NULL, victim, TO_CHAR);
	act("$n swings at you, but you avoid the blow!", ch, NULL, victim, TO_VICT);
	act("$n swings at $N, but $E avoids the blow!", ch, NULL, victim, TO_NOTVICT);
	damage(ch, victim, 0, gsn_disruption, wield->value[3], FALSE);
	check_improve(ch,victim, gsn_disruption, FALSE, 1);
	return;
    }

    check_improve(ch,victim, gsn_disruption, TRUE, 1);

    dam = td = 0;
    
    if (GET_DAMROLL(ch) > 80)
    {
	td = GET_DAMROLL(ch) - 80;
	td = round(td/5);
    }
    if (td+80 < GET_DAMROLL(ch))
	dam = number_range(GET_DAMROLL(ch),GET_DAMROLL(ch)*1.25);
    else 
	dam = number_range(GET_DAMROLL(ch),(td ? td+80 : GET_DAMROLL(ch))*2);

    dam = round(dam * UMIN(get_weapon_skill_weapon(ch,wield),100)/100);

    damage_from_obj( ch, victim, wield, dam, gsn_disruption, wield->value[3], TRUE );
    
    if ((mantle = get_eq_char(victim, WEAR_ABOUT))
     && (mantle->pIndexData->vnum == OBJ_VNUM_MANTLE_EARTH)
     && (number_bits(1) == 0))
    {
	act("$N's mantle of earth shatters as you strike it!", ch, NULL, victim, TO_CHAR);
	act("Your mantle of earth shatters as $n strikes it!", ch, NULL, victim, TO_VICT);
	act("$N's mantle of earth shatters as $n strikes it!", ch, NULL, victim, TO_NOTVICT);
	extract_obj(mantle);
    }

    if (check_dispel(ch->level, victim, gsn_fortify))
	act("$n looks less healthy.",victim, NULL, NULL,TO_ROOM);
    if (check_dispel(ch->level, victim, gsn_giantstrength))
	act("$n no longer looks so mighty.",victim,NULL,NULL,TO_ROOM);
    if (check_dispel(ch->level, victim, gsn_stoneskin))
	act("$n's skin regains its normal texture.",victim,NULL,NULL,TO_ROOM);
    if (check_dispel(ch->level, victim, gsn_stoneshell))
	act("Your stone shell crumbles.",victim,NULL,NULL,TO_CHAR);
    if (check_dispel(ch->level, victim, gsn_anchor))
	act("$n no longer stands so steadfast.",victim,NULL,NULL,TO_ROOM);

    return;
}	    
