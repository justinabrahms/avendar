#if defined(macintosh)
#include <types.h>
#else
#include <sys/types.h>
#endif
#include "spells_earth.h"
#include <stdio.h>
#include <time.h>
#include <sstream>
#include "magic.h"
#include "lookup.h"
#include "Runes.h"
#include "NameMaps.h"
#include "recycle.h"
#include "EchoAffect.h"
#include "Weave.h"
#include "Drakes.h"
#include "Luck.h"
#include "spells_fire.h"
#include "spells_void.h"
#include "spells_spirit_earth.h"

/* External declarations */
DECLARE_DO_FUN(do_look);
DECLARE_DO_FUN(do_autoyell);

extern	void 		free_exit	args( ( EXIT_DATA *pExit ) );
extern  EXIT_DATA 	*new_exit	args( ( void ) );
extern	int		hands_free	args( ( CHAR_DATA *ch ) );

extern	bool	check_aura	args( ( CHAR_DATA *ch, OBJ_DATA *obj, bool damage ) );

bool is_stabilized(const CHAR_DATA & ch)
{
    if (ch.in_room == NULL || !ON_GROUND(&ch)) return false;
    if (is_flying(&ch)) return false;
    return is_affected(&ch, gsn_stabilize);
}

bool spell_clayshield(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    // Check for cooldown
    CHAR_DATA * victim(static_cast<CHAR_DATA*>(vo));
    AFFECT_DATA * paf(get_affect(victim, sn));
    if (paf != NULL)
    {
        if (paf->modifier == 0)
        {
            if (ch == victim) send_to_char("You are already guarded by a clay shield.\n", ch);
            else act("$N is already guarded by a clay shield.\n", ch, NULL, victim, TO_CHAR);
        }
        else
        {
            if (ch == victim) send_to_char("You are not yet ready to be warded by another clay shield.\n", ch);
            else act("$N is not yet ready to be warded by another clay shield.", ch, NULL, victim, TO_CHAR);
        }
        return false;
    }

    // Echoes
    act("A shield of earth and clay forms about you.", victim, NULL, NULL, TO_CHAR);
    act("A shield of earth and clay forms about $n.", victim, NULL, NULL, TO_ROOM);

    // Apply effect
    AFFECT_DATA af = {0};
    af.where    = TO_AFFECTS;
    af.type     = sn;
    af.level    = level;
    af.duration = 9 + level + (check_durablemagics(*ch) ? 20 : 0);
    affect_to_char(victim, &af);
    return true;
}

void check_mudfootcurse(CHAR_DATA & ch)
{
    // Check disqualifiers
    if (ch.in_room == NULL || !ON_GROUND(&ch) || number_bits(3) != 0 || is_flying(&ch) 
    || !is_affected(&ch, gsn_mudfootcurse) || is_stabilized(ch))
        return;

    bool fighting(ch.position == POS_FIGHTING);
    if (!fighting && number_bits(1) != 0 && ch.position == POS_STANDING)
        return;

    // Check for a dex save
    if (number_percent() > (get_curr_stat(&ch, STAT_DEX) * 2))
    {
        send_to_char("You slip on the mud coating your feet, but manage to catch your balance.\n", &ch);
        return;
    }

    // Slip them
    act("You slip on the mud coating your feet, falling to the ground!", &ch, NULL, NULL, TO_CHAR);
    act("$n slips on the mud coating $s feet, falling to the ground!", &ch, NULL, NULL, TO_ROOM);
    switch_position(&ch, POS_SITTING);
    WAIT_STATE(&ch, PULSE_VIOLENCE);

    // Check for rolling downhills
    if (fighting || number_bits(1) != 0) return;
    if (IS_NPC(&ch) && (IS_SET(ch.act, ACT_SENTINEL) || IS_SET(ch.act, ACT_NOSUBDUE) || IS_SET(ch.nact, ACT_SHADE))) return;

    ROOM_INDEX_DATA * downhill(Direction::Adjacent(*ch.in_room, Direction::Down, EX_CLOSED|EX_WALLED|EX_WALLOFVINES|EX_WEAVEWALL|EX_ICEWALL));
    if (downhill == NULL)
        return;

    act("The momentum of your fall takes you off-guard, and you go rolling downwards!", &ch, NULL, NULL, TO_CHAR);
    act("The momentum of $s fall takes $m off-guard, and $e goes rolling downwards!", &ch, NULL, NULL, TO_ROOM);

    char_from_room(&ch);
    char_to_room(&ch, downhill);
    act("$n comes rolling in from above, tumbling wildly!", &ch, NULL, NULL, TO_ROOM);
    do_look(&ch, "auto");
}

bool spell_mudfootcurse(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    // Check whether already cursed
    CHAR_DATA * victim(static_cast<CHAR_DATA*>(vo));
    if (is_affected(victim, sn))
    {
        act("$N is already afflicted with a mudfoot curse.", ch, NULL, victim, TO_CHAR);
        return false;
    }

    // Check for a save
    if (saves_spell(level, ch, victim, DAM_OTHER))
    {
        act("Mud starts to build up on $N, but $E darts clear of it!", ch, NULL, victim, TO_CHAR);
        act("Mud starts to build up on you, but you dart clear of it!", ch, NULL, victim, TO_VICT);
        return true;
    }

    // Echoes
    act("Mud builds up on you, impeding your motion!", victim, NULL, NULL, TO_CHAR);
    act("Mud builds up on $n, impeding $s motion!", victim, NULL, NULL, TO_ROOM);

    // Apply effect
    AFFECT_DATA af = {0};
    af.where    = TO_AFFECTS;
    af.type     = sn;
    af.level    = level;
    af.duration = (level * (check_durablemagics(*ch) ? 2 : 1)) / 2;
    af.location = APPLY_DEX;
    af.modifier = -2;
    affect_to_char(victim, &af);
    return true;
}

bool spell_reforgemagic(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    // Verify the object
    OBJ_DATA * obj(static_cast<OBJ_DATA*>(vo));
    if (obj->item_type != ITEM_STAFF)
    {
        send_to_char("You can only reforge the magic within staves.\n", ch);
        return false;
    }

    if (obj_is_affected(obj, sn))
    {
        act("The magic within $p has already been reforged.", ch, obj, NULL, TO_CHAR);
        return false;
    }

    // Check the argument
    if (target_name[0] == '\0')
    {
        send_to_char("Did you want to concentrate the magics or make them more diffuse?\n", ch);
        return false;
    }

    // Determine the type of reforging
    int skill(get_skill(ch, sn));
    if (!str_prefix(target_name, "concentrate"))
    {
        // Make sure there are at least two charges left
        if (obj->value[2] < 2)
        {
            act("$p has insufficient charges left to concentrate them.", ch, obj, NULL, TO_CHAR);
            return false;
        }

        // Burn a charge to increase the level
        --obj->value[2];
        obj->value[0] += 6 + (level / 17) + number_range(0, 3) + (UMAX(0, skill - 70) / 10);
        act("You reforge the magics in $p, concentrating its charges into more powerful workings.", ch, obj, NULL, TO_CHAR);
    }
    else if (!str_prefix(target_name, "diffuse"))
    {
        // Make sure there are at least some levels to cut away
        if (obj->value[0] < 15)
        {
            act("The magics within $p are too weak to diffuse any further.", ch, obj, NULL, TO_CHAR);
            return false;
        }

        // Burn some levels to increase the charge
        ++obj->value[2];
        obj->value[1] = UMAX(obj->value[1], obj->value[2]);
        obj->value[0] -= 18 - number_range(0, 3) - (UMAX(0, skill - 70) / 10);
        act("You reforge the magics in $p, diffusing its power into more charges.", ch, obj, NULL, TO_CHAR);
    }
    else
    {
        send_to_char("You only know how to concentrate or diffuse such magics.\n", ch);
        return false;
    }
    act("$n passes a palm over $p, which hums in response.", ch, obj, NULL, TO_ROOM);

    // Luck can swing the levels further either way
    switch (Luck::Check(*ch))
    {
        case Luck::Lucky: obj->value[0] += 5; break;
        case Luck::Unlucky: obj->value[0] -= 5; break;
        default: break;
    }
    obj->value[0] = UMAX(obj->value[0], 1);

    // Apply effect to prevent more reforging
    AFFECT_DATA af = {0};
    af.where    = TO_OBJECT;
    af.type     = sn;
    af.level    = level;
    af.location = APPLY_HIDE;
    af.duration = -1;
    affect_to_obj(obj, &af);
    return true;
}

static OBJ_DATA * lookup_tuning_stone(ROOM_INDEX_DATA & room)
{
    // If the room effect is absent, there is no valid tuning stone
    if (!room_is_affected(&room, gsn_tuningstone))
        return NULL;

    // Room effect is present, seek the matching tuning stone
    for (OBJ_DATA * obj(object_list); obj != NULL; obj = obj->next)
    {
        AFFECT_DATA * paf(get_obj_affect(obj, gsn_tuningstone));
        if (paf != NULL && paf->modifier == room.vnum)
            return obj;
    }

    // Found no tuning stone, so clean up the room effect
    room_affect_strip(&room, gsn_tuningstone);
    return NULL;
}

void check_tuningstone(CHAR_DATA & ch, ROOM_INDEX_DATA & room)
{
    // Look up the tuning stone for this room
    OBJ_DATA * obj(lookup_tuning_stone(room));
    if (obj == NULL)
        return;

    // Found the tuning stone, look up its containing room
    ROOM_INDEX_DATA * stoneRoom(get_room_for_obj(*obj));
    if (stoneRoom == NULL)
        return;

    // Echo
    act("$p suddenly hums to life, vibrating of its own accord.", stoneRoom->people, obj, NULL, TO_ALL);
}

bool spell_tuningstone(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    // Check for cooldown
    if (is_affected(ch, sn))
    {
        send_to_char("You are not yet ready to draw the earth's power into a tuning stone again.\n", ch);
        return false;
    }

    // Check the room
    if (ch->in_room == NULL || !ON_GROUND(ch))
    {
        send_to_char("There is no ground here to which to tune the stone!\n", ch);
        return false;
    }

    // Check whether the room is already tuned
    if (lookup_tuning_stone(*ch->in_room) != NULL)
    {
        send_to_char("This place has already been tuned to a stone.\n", ch);
        return false;
    }

    // Check whether object is already tuned
    OBJ_DATA * obj(static_cast<OBJ_DATA*>(vo));
    if (obj_is_affected(obj, sn))
    {
        act("$p has already been tuned to a location.", ch, obj, NULL, TO_CHAR);
        return false;
    }

    // Check object material
    if (!material_table[obj->material].stone)
    {
        send_to_char("Only items composed of some type of stone may be properly tuned to the earth.\n", ch);
        return false;
    }

    // Echoes
    act("You touch $p lightly to the ground, tuning it to the resonances of this place.", ch, obj, NULL, TO_CHAR);
    act("$n touches $p lightly to the ground, murmuring softly.", ch, obj, NULL, TO_ROOM);

    // Apply effects
    AFFECT_DATA af = {0};
    af.where    = TO_ROOM_AFF;
    af.level    = level;
    af.type     = sn;
    af.duration = 9 + level + (check_durablemagics(*ch) ? 20 : 0);
    affect_to_room(ch->in_room, &af);

    af.where    = TO_OBJECT;
    af.modifier = ch->in_room->vnum;
    affect_to_obj(obj, &af);

    // Apply cooldown
    int skill(get_skill(ch, sn));
    af.where    = TO_AFFECTS;
    af.duration = 22 - (UMAX(0, skill - 70) / 5);
    affect_to_char(ch, &af);
    return true;
}

static void reinforce_object(CHAR_DATA & ch, int level, OBJ_DATA & obj, int divisor)
{
    // Prepare basic effect
    AFFECT_DATA af = {0};
    af.where    = TO_OBJECT;
    af.type     = gsn_reinforce;
    af.level    = level;
    af.duration = level * (check_durablemagics(ch) ? 2 : 1);
    af.bitvector = ITEM_NODESTROY;

    // Strip any existing effect
    object_affect_strip(&obj, gsn_reinforce);

    // Calculate how much AC is present on the item naturally
    int totalAC(0);
    if (obj.item_type == ITEM_ARMOR)
    {
        // Tally the natural AC
        for (unsigned int i(0); i <= 3; ++i)
            totalAC -= obj.value[i];
    }

    // Tally the effect AC
    for (const AFFECT_DATA * paf(obj.affected); paf != NULL; paf = paf->next)
    {
        if (paf->location == APPLY_AC)
            totalAC += paf->modifier;
    }

    // Calculate the AC bonus and apply the effect
    af.modifier = totalAC / divisor;
    af.modifier = UMIN(-1, af.modifier);
    af.location = APPLY_AC;
    affect_to_obj(&obj, &af);
}

static void reinforce_person(CHAR_DATA & ch, int level, CHAR_DATA & victim, int divisor)
{
    // Iterate the objects carried by the target
    for (OBJ_DATA * obj(victim.carrying); obj != NULL; obj = obj->next_content)
    {
        // If the item is worn, reinforce it
        if (obj->worn_on)
            reinforce_object(ch, level, *obj, divisor);
    }
}

static int reinforce_divisor(CHAR_DATA & ch)
{
    if (number_percent() <= get_skill(&ch, gsn_forgemaster))
    {
        check_improve(&ch, NULL, gsn_forgemaster, true, 16);
        return 4;
    }
    
    check_improve(&ch, NULL, gsn_forgemaster, false, 16);
    return 6;
}

bool spell_reinforce(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    // Check for a target
    if (target_name[0] == '\0')
    {
        // No target, so reinforce the caster's gear
        send_to_char("You reinforce your equipment with earthen magics, hardening it against blows.\n", ch);
        reinforce_person(*ch, level, *ch, reinforce_divisor(*ch));
        return true; 
    }

    // Check whether the supplied target is a person in the room
    CHAR_DATA * victim(get_char_room(ch, target_name));
    if (victim != NULL)
    {
        // Check whether a groupmate
        if (!is_same_group(ch, victim))
        {
            send_to_char("You may only reinforce the armor of groupmates.\n", ch);
            return false;
        }

        // Reinforce the target's gear
        act("You gesture towards $N, reinforcing $S equipment with earthen power.", ch, NULL, victim, TO_CHAR);
        act("$n gestures towards you, and you feel your equipment toughen in response.", ch, NULL, victim, TO_VICT);
        act("$n gestures towards $N as $e murmurs an arcane word.", ch, NULL, victim, TO_NOTVICT);
        reinforce_person(*ch, level, *victim, reinforce_divisor(*ch));
        return true;
    }

    // Check whether the supplied target is an inventory object
    OBJ_DATA * obj(get_obj_carry(ch, target_name, ch));
    if (obj != NULL)
    {
        // Reinforce the single object
        act("You reinforce $p with earthen magics, hardening it against blows.", ch, obj, NULL, TO_CHAR);
        reinforce_object(*ch, level, *obj, reinforce_divisor(*ch));
        return true;
    }

    // Target was supplied but is not valid
    send_to_char("You see no one and carry nothing by that name here.\n", ch);
    return false;
}

bool spell_honeweapon(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    // Verify the object is a weapon
    OBJ_DATA * obj(static_cast<OBJ_DATA*>(vo));
    if (obj->item_type != ITEM_WEAPON)
    {
        send_to_char("You can only hone weapons.\n", ch);
        return false;
    }

    // Verify the material
    if (!material_table[obj->material].stone && !material_table[obj->material].metal)
    {
        send_to_char("You only know how to hone stone or metal weapons.\n", ch);
        return false;
    }

    // Verify the weapon is not already honed
    if (obj_is_affected(obj, sn))
    {
        act("$p has already been honed.", ch, obj, NULL, TO_CHAR);
        return false;
    }

    // Determine level mod
    int levelMod(-2);
    if (number_percent() <= get_skill(ch, gsn_forgemaster))
    {
        levelMod = 2;
        check_improve(ch, NULL, gsn_forgemaster, true, 12);
    }
    else
        check_improve(ch, NULL, gsn_forgemaster, false, 12);

    // Prepare effect
    AFFECT_DATA af = {0};
    af.where    = TO_OBJECT;
    af.type     = sn;
    af.level    = level;
    af.duration = 90;
    af.modifier = UMAX(1, (level + levelMod) / 17);

    // Handle by damage type
    switch (obj->value[3])
    {
        case DAM_PIERCE:
        case DAM_SLASH: af.location = APPLY_DAMROLL; break;
        case DAM_BASH: af.location = APPLY_HITROLL; break;
        default: send_to_char("You can only hone weapons which do physical damage.\n", ch); return false;
    }
    affect_to_obj(obj, &af);
    obj->condition = 100;

    // Echoes
    act("With practiced ease you hone $p, repairing and improving it.", ch, obj, NULL, TO_CHAR);
    act("With practiced ease $n hones $p, repairing and improving it.", ch, obj, NULL, TO_ROOM);
    return true;
}

bool spell_shellofstone(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    return spell_stoneshell(sn, level, ch, vo, target);
}

bool spell_rocktomud(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    return spell_stonetomud(sn, level, ch, vo, target);
}

bool spell_crush(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    struct Info
    {
        long victim_id;
        int level;
    };

    struct CallbackHandler
    {
        static bool HandleMove(CHAR_DATA * ch, ROOM_INDEX_DATA *, EchoAffect *, void * tag) {return HandleCancel(ch, tag);}
        static bool HandleCast(CHAR_DATA * ch, int, int, void *, int, EchoAffect *, void * tag) {return HandleCancel(ch, tag);}

        static bool HandlePositionChange(CHAR_DATA * ch, int newPos, EchoAffect *, void * tag) 
        {
            if (newPos == POS_FIGHTING)
                return false;

            return HandleCancel(ch, tag);
        }

        static bool HandleCheck(CHAR_DATA * ch, EchoAffect *, void * tag) 
        {
            CHAR_DATA * victim(LookupVictim(ch, tag));
            if (victim == NULL)
                return HandleCancel(ch, tag);

            act("The air about you begins to hum with energy as you tighten your grip, directing your power at $N!", ch, NULL, victim, TO_CHAR);
            act("The air about $n begins to hum with energy as a powerful force grips you tightly!", ch, NULL, victim, TO_VICT);
            act("The air about $n begins to hum with energy.", ch, NULL, victim, TO_NOTVICT);
            return false;
        }

        static bool Finish(CHAR_DATA * ch, EchoAffect * echoAff, void * tag)
        {
            CHAR_DATA * victim(LookupVictim(ch, tag));
            if (victim == NULL)
                return HandleCancel(ch, tag);

            // Echoes
            act("You squeeze the earthen power about $N, crushing $M painfully!", ch, NULL, victim, TO_CHAR);
            act("Bands of power tighten about you, crushing you painfully!", ch, NULL, victim, TO_VICT);
            Info * info(static_cast<Info*>(tag));

            // Deal damage
            bool cancelEffect(false);
            bool saved(false);
            int dam(dice(info->level, 9));
            if (saves_spell(info->level, ch, victim, DAM_BASH))
            {
                // Saved once; another save cancels the effect (after this round)
                // It gets easier and easier to save, as the level drops each time
                dam /= 2;
                saved = true;
                info->level = UMAX(1, info->level - 2);
                if (saves_spell(info->level, ch, victim, DAM_BASH))
                    cancelEffect = true;
            }
            info->level = UMAX(1, info->level - 2);
            damage(ch, victim, dam, gsn_crush, DAM_BASH, true);

            // Apply or intensify effect
            if (!saved)
            {
                AFFECT_DATA * paf(get_affect(victim, gsn_crush));
                int duration(check_durablemagics(*ch) ? 12 : 8);
                if (paf == NULL)
                {
                    AFFECT_DATA af = {0};
                    af.where    = TO_AFFECTS;
                    af.type     = gsn_crush;
                    af.level    = info->level;
                    af.duration = duration;
                    af.modifier = 10;
                    affect_to_char(victim, &af);

                    send_to_char("You gasp and wheeze, choking as the breath is squeezed from you!\n", victim);
                    act("$n gasps and wheezes, choking as the breath is squeezed from $m!", victim, NULL, NULL, TO_ROOM);
                }
                else
                {
                    paf->duration = UMAX(duration, paf->duration);
                    paf->modifier += number_range(1, 5);
                    paf->modifier = UMIN(paf->modifier, 50);
                }
            
                // Handle salt of the earth modifier
                if (!is_affected(victim, gsn_agony) && number_percent() <= determine_saltoftheearth_level(*ch, SPH_VOID))
                {
                    AFFECT_DATA af = {0};
                    af.where     = TO_AFFECTS;
                    af.type      = gsn_agony;
                    af.level     = info->level;
                    af.duration  = info->level / 11;
                    af.location  =  APPLY_STR;
                    af.modifier  = -1 * (info->level / 12);
                    affect_to_char(victim, &af);

                    send_to_char("You scream in agony as pain wracks your body!\n", victim);
                    act("$n screams in agony as pain wracks $s body!", victim, NULL, NULL, TO_ROOM);
                }
            }

            // Clean up or set up for another go
            if (cancelEffect) 
            {
                delete info;
                send_to_char("With an effort of will, you throw off the crushing earthen power!\n", victim);
                act("$N slips free of your crushing magics.", ch, NULL, victim, TO_CHAR); 
            }
            else 
                echoAff->AddLine(&CallbackHandler::Finish, "");

            return cancelEffect;
        }

        private:
            static CHAR_DATA * LookupVictim(CHAR_DATA * ch, void * tag)
            {
                if (ch->in_room == NULL)
                    return NULL;

                return get_char_by_id_any_room((static_cast<Info*>(tag))->victim_id, *ch->in_room);
            }

            static bool HandleCancel(CHAR_DATA * ch, void * tag)
            {
                delete static_cast<Info*>(tag);
                send_to_char("You release your grip, letting the energies flow away.\n", ch);
                return true;
            }
    };

    // Echoes
    CHAR_DATA * victim(static_cast<CHAR_DATA*>(vo));
    act("Holding a palm out towards $N, you clench your fist loosely, drawing in power.", ch, NULL, victim, TO_CHAR);
    act("$n holds a palm out towards you, clenching $s fist loosely.", ch, NULL, victim, TO_VICT);
    act("$n holds a palm out at towards $N, clenching $s fist loosely.", ch, NULL, victim, TO_NOTVICT);

    // Prepare the echo affect
    EchoAffect * echoAff(new EchoAffect(1));
    echoAff->SetPositionCallback(&CallbackHandler::HandlePositionChange);
    echoAff->SetMoveCallback(&CallbackHandler::HandleMove);
    echoAff->SetCastCallback(&CallbackHandler::HandleCast);

    // Add in lines
    echoAff->AddLine(&CallbackHandler::HandleCheck, "");
    echoAff->AddLine(&CallbackHandler::Finish, "");

    // Finish applying effect
    Info * info = new Info();
    info->level = level;
    info->victim_id = victim->id;
    echoAff->SetTag(info);
    EchoAffect::ApplyToChar(ch, echoAff);
    return true;
}

static OBJ_DATA * create_quarry_gem(int level)
{
    OBJ_DATA * obj(create_object(get_obj_index(OBJ_VNUM_QUARRYGEM), 0));

    std::ostringstream shortDesc;
    switch (number_range(0, 7))
    {
        case 0: shortDesc << "a glittering"; break;
        case 1: shortDesc << "a shimmering"; break;
        case 2: shortDesc << "a sparkling"; break;
        case 3: shortDesc << "a gleaming"; break;
        case 4: shortDesc << "a polished"; break;
        case 5: shortDesc << "an evenly-faceted"; break;
        case 6: shortDesc << "a shining"; break;
        default: shortDesc << "a shimmering"; break;
    }

    const char * material(NULL);
    switch (number_range(0, 8))
    {
        case 0: material = "diamond";       obj->cost = dice(level, 100);   break;
        case 1: material = "ruby";          obj->cost = dice(level, 80);    break;
        case 3: material = "sapphire";      obj->cost = dice(level, 80);    break;
        case 2: material = "emerald";       obj->cost = dice(level, 60);    break;
        case 4: material = "topaz";         obj->cost = dice(level, 60);    break;
        case 5: material = "amethyst";      obj->cost = dice(level, 40);    break;
        case 6: material = "aquamarine";    obj->cost = dice(level, 40);    break;
        case 7: material = "opal";          obj->cost = dice(level, 40);    break;
        default: material = "gem";          obj->cost = dice(level, 20);    break;
    }

    obj->material = material_lookup(material);
    if (obj->material < 0)
    {
        bug("Failed to find material in quarry gem lookup", 0);
        extract_obj(obj);
        return NULL;
    }

    shortDesc << " " << material;

    // Set up the strings
    setName(*obj, shortDesc.str().c_str());
    copy_string(obj->short_descr, shortDesc.str().c_str());

    std::string longDesc(shortDesc.str());
    longDesc += " is here, sparkling vividly.";
    longDesc[0] = UPPER(longDesc[0]);
    copy_string(obj->description, longDesc.c_str());

    // Set up the rest of the gem
    obj->level = level;
    obj->material = UMAX(obj->material, 0);
    obj->weight = number_range(2, 12);
    return obj;
}

static void add_quarry_bonus(OBJ_DATA & obj, int level)
{
    // Prepare basic effect
    AFFECT_DATA af = {0};
    af.where    = TO_OBJECT;
    af.type     = -1;
    af.level    = level;
    af.duration = -1;

    // Determine random bonus type
    switch (number_range(0, 12))
    {
        case 0: af.location = APPLY_HIT; af.modifier = 3 + number_range(0, level / 5); break;
        case 1: af.location = APPLY_MANA; af.modifier = number_range(1, level / 5); break;
        case 2: af.location = APPLY_MOVE; af.modifier = number_range(1, level / 3) - number_range(0, level / 3); break;
        case 3: af.location = APPLY_HITROLL; af.modifier = number_range(1, level / 17); break;
        case 4: af.location = APPLY_DAMROLL; af.modifier = number_range(0, level / 20); break;
        case 5: af.location = APPLY_RESIST_WEAPON; af.modifier = number_range(0, level / 20); break;
        case 6: af.location = APPLY_SAVES; af.modifier = -(number_range(1, level / 17)); break;
        case 7: af.location = APPLY_LUCK; af.modifier = number_range(0, 5); break;
    }

    // Apply the effect
    if (af.modifier != 0)
        obj_affect_join(&obj, &af);
}

static OBJ_DATA * create_quarry_item(int level, int stoneType, int ch_id)
{
    // Small chance of a gem instead
    if (number_percent() <= 5)
        return create_quarry_gem(level);
   
    // Make the object 
    OBJ_DATA * obj(create_object(get_obj_index(OBJ_VNUM_QUARRYARMOR), 0));

    int wearBit, baseSize, baseWeight;
    const char * name;
    bool plural(false);
    switch (number_range(0, 6))
    {
        case 0:  wearBit = ITEM_WEAR_BODY;   name = "breastplate"; baseSize = SIZE_LARGE;  baseWeight = 70; break;
        case 1:  wearBit = ITEM_WEAR_HEAD;   name = "helmet";      baseSize = SIZE_MEDIUM; baseWeight = 40; break;
        case 2:  wearBit = ITEM_WEAR_LEGS;   name = "greaves";     baseSize = SIZE_MEDIUM; baseWeight = 40; plural = true; break;
        case 3:  wearBit = ITEM_WEAR_FEET;   name = "boots";       baseSize = SIZE_MEDIUM; baseWeight = 40; plural = true; break;
        case 4:  wearBit = ITEM_WEAR_ARMS;   name = "armguards";   baseSize = SIZE_MEDIUM; baseWeight = 30; plural = true; break;
        case 5:  wearBit = ITEM_WEAR_SHIELD; name = "shield";      baseSize = SIZE_MEDIUM; baseWeight = 60; break;
        default: wearBit = ITEM_WEAR_NECK;   name = "neckplate";   baseSize = SIZE_SMALL;  baseWeight = 30; break;
    }

    const char * adjective(NULL);
    int acBase(5);
    switch (number_range(0, 13))
    {
        case 0: adjective = "light"; baseWeight -= 30; acBase = 0; break;
        case 1: adjective = "heavy"; baseWeight += 30; acBase += 5; break;
        case 2: adjective = "dark";  SET_BIT(obj->extra_flags[0], ITEM_DARK); break;
        case 3: adjective = "gleaming"; SET_BIT(obj->extra_flags[0], ITEM_GLOW); break;
        case 4: adjective = "tough"; SET_BIT(obj->extra_flags[0], ITEM_NODESTROY); break;
        case 5: adjective = "polished"; break;
        case 6: adjective = "rough-hewn"; break;
        case 7: adjective = "solid"; break;
        case 8: adjective = "smooth"; break;
        case 9: adjective = "scratched"; break;
        case 10: adjective = "jagged"; break;
        case 11: adjective = "massive"; baseSize = SIZE_HUGE; break;
    }

    // Set initial values
    SET_BIT(obj->wear_flags, wearBit);
    obj->level = level;
    obj->material = stoneType;
    obj->size = baseSize;
    obj->weight = baseWeight + number_range(0, 10);
    obj->cost = dice(level, 5);
    
    // Build the short desc
    std::ostringstream temp;
    if (adjective != NULL)
        temp << adjective << ' ';

    temp << material_table[stoneType].name << ' ' << name;
    std::string shortDesc(temp.str());
    std::string article(plural ? "some" : indefiniteArticleFor(shortDesc[0]));
    shortDesc = article + ' ' + shortDesc;
    
    // Set the strings
    setName(*obj, shortDesc.c_str());
    copy_string(obj->short_descr, shortDesc.c_str());

    std::string longDesc(shortDesc + " lies here.");
    longDesc[0] = UPPER(longDesc[0]);
    copy_string(obj->description, longDesc.c_str());

    // Determine AC values
    obj->value[0] = acBase + number_range(2, 10);
    obj->value[1] = acBase + number_range(2, 10);
    obj->value[2] = acBase + number_range(2, 10);
    obj->value[3] = acBase + number_range(0, 2);

    // Give 1 bonus based on the stone type and char
    srand(ch_id + stoneType + 12345);
    add_quarry_bonus(*obj, level);
    srand(time(0));

    // Give 0 to 2 random bonuses
    int bonusCount(number_range(0, 2));
    for (int i(0); i < bonusCount; ++i)
        add_quarry_bonus(*obj, level);

    return obj;
}

bool spell_quarry(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    // Check for cooldown
    if (is_affected(ch, sn))
    {
        send_to_char("You are not ready to quarry the earth again just yet.\n", ch);
        return false;
    }

    // Check the room
    if (ch->in_room == NULL || !ON_GROUND(ch))
    {
        send_to_char("There is no ground here to quarry.\n", ch);
        return false;
    }

    // Get the stone type for the room
    int stoneType(ch->in_room->stone_type);
    if (stoneType < 0) stoneType = ch->in_room->area->stone_type;
    if (stoneType < 0 || stoneType >= MAX_MATERIALS || !material_table[stoneType].stone)
    {
        stoneType = material_lookup("stone");
        if (stoneType < 0)
        {
            bug("No material stone found in quarry lookup", 0);
            send_to_char("This place is unsuitable for quarrying.\n", ch);
            return false;
        }
    }

    // Make the object
    OBJ_DATA * obj(create_quarry_item(level, stoneType, ch->id));
    if (obj == NULL)
    {
        send_to_char("An error has occurred, please contact the gods.\n", ch);
        return false;
    }
    obj_to_room(obj, ch->in_room);

    // Echoes
    act("You extend your will into the ground, turning up chunks of stone as you quarry the earth!", ch, NULL, NULL, TO_CHAR);
    act("$n holds $s hands out over the earth, which begins to churn, spewing out stones as $e quarries the land!", ch, NULL, NULL, TO_ROOM);
    act("When the dust settles, $p remains, perched atop a pile of debris.", ch, obj, NULL, TO_ALL);

    // Apply cooldown
    int skill(get_skill(ch, sn));
    AFFECT_DATA af = {0};
    af.where    = TO_AFFECTS;
    af.type     = sn;
    af.level    = level;
    af.duration = 120 - (UMAX(0, skill - 70) * 2);
    affect_to_char(ch, &af);
    return true;
}

static void create_wallofstonehelper(ROOM_INDEX_DATA & room, Direction::Value direction, int level)
{
    // Check for existing wall of stone
    if (IS_SET(room.exit[direction]->exit_info, EX_WALLED))
        return;

    // Echo about it
    std::ostringstream mess;
    mess << "With a tear of stone, a wall of solid rock rises to seal off the exit ";
    mess << Direction::DirectionalNameFor(direction) << ".";
    act(mess.str().c_str(), room.people, NULL, NULL, TO_ALL);

    // Set the bit
    SET_BIT(room.exit[direction]->exit_info, EX_WALLED);
}

// Assumes there is a valid exit in the direction specified
static void create_wallofstone(CHAR_DATA * ch, ROOM_INDEX_DATA & room, Direction::Value direction, int level)
{
    // Apply to this room
    create_wallofstonehelper(room, direction, level);

    // Apply to the adjacent room if appropriate
    Direction::Value opposite(Direction::ReverseOf(direction));
    ROOM_INDEX_DATA * nextRoom(Direction::Adjacent(room, direction));
    if (nextRoom != NULL && Direction::Adjacent(*nextRoom, opposite) == &room)
        create_wallofstonehelper(*nextRoom, opposite, level);

    // Add the effect
    AFFECT_DATA af = {0};
    af.where	 = TO_ROOM_AFF;
    af.type      = gsn_wallofstone;
    af.level     = level;
    af.duration  = (level / 3) + ((ch != NULL && check_durablemagics(*ch)) ? 10 : 0);
    af.modifier  = direction;
    affect_to_room(&room, &af);
}

void check_glyphofentombment(ROOM_INDEX_DATA & room, Direction::Value direction)
{
    // Get the effect
    AFFECT_DATA * paf(get_room_affect(&room, gsn_glyphofentombment));
    if (paf == NULL || paf->modifier != direction)
        return;
   
    // Get the adjacent room, if any
    ROOM_INDEX_DATA * nextRoom(Direction::Adjacent(room, static_cast<Direction::Value>(paf->modifier)));
    if (nextRoom != NULL)
    {
        act("The lines of the glyph of entombment flash with power!", nextRoom->people, NULL, NULL, TO_ALL);
        room_affect_strip(nextRoom, gsn_glyphofentombment);
    }
         
    // Put up the wall
    act("The lines of the glyph of entombment flash with power!", room.people, NULL, NULL, TO_ALL);
    create_wallofstone(NULL, room, static_cast<Direction::Value>(paf->modifier), paf->level);
    room_affect_strip(&room, gsn_glyphofentombment);
}

bool spell_glyphofentombment(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    // Check for cooldown
    if (is_affected(ch, sn))
    {
        send_to_char("You are not ready to empower another glyph of entombment yet.\n", ch);
        return false;
    }

    // Check the room
    if (ch->in_room == NULL || !ON_GROUND(ch))
	{
        send_to_char("There is no ground here on which to scribe such a glyph.\n", ch);
        return false;
    }

    if (room_is_affected(ch->in_room, sn))
    {
        send_to_char("This place has already been marked with a glyph of entombment.\n", ch);
        return false;
    }

    // Get the direction
    if (target_name[0] == '\0')
    {
        send_to_char("In which direction did you wish to scribe the glyph?\n", ch);
        return false;
    }

    Direction::Value direction(Direction::ValueFor(target_name));
    if (direction == Direction::Max)
    {
        send_to_char("That's not a valid direction.\n", ch);
        return false;
    }

    // Verify the direction
    EXIT_DATA * exitData(ch->in_room->exit[direction]);
    if (exitData == NULL)
    {
        send_to_char("There's no exit that way!\n", ch);
        return false;
    }

    if (IS_SET(exitData->exit_info, EX_WALLED))
    {
    	send_to_char("A wall of stone already blocks that way.\n", ch);
	    return false;
    }

    // Echoes
    std::ostringstream mess;
    mess << "You trace the interlocking lines of a glyph of entombment on the ground, pointing it ";
    mess << Direction::DirectionalNameFor(direction) << ".\n";
    send_to_char(mess.str().c_str(), ch);
    act("$n traces an intricate glyph on the ground, scribing its many interlocking lines with care.", ch, NULL, NULL, TO_ROOM);

    // Apply effect
    AFFECT_DATA af = {0};
    af.where	= TO_ROOM_AFF;
    af.type	    = sn;
    af.level    = level;
    af.duration = 3 + (level / 3);
    af.modifier	= direction;
    affect_to_room(ch->in_room, &af);

    // Apply effect to the opposite room, if applicable
    Direction::Value opposite(Direction::ReverseOf(direction));
    ROOM_INDEX_DATA * room(Direction::Adjacent(*ch->in_room, direction));
    if (room != NULL && Direction::Adjacent(*room, opposite) == ch->in_room)
    {
        // Exits are sane, apply effect
        af.modifier = opposite;
        affect_to_room(room, &af);
    }

    // Apply cooldown
    int skill(get_skill(ch, sn));
    af.where    = TO_AFFECTS;
    af.duration = 40 - (UMAX(0, skill - 70) / 3);
    affect_to_char(ch, &af);
    return true;

    return true;
}

bool spell_latticeofstone(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    // Check for cooldown
    if (is_affected(ch, sn))
    {
        send_to_char("You are not ready to scribe another stone lattice yet.\n", ch);
        return false;
    }

    // Check for room
    if (ch->in_room == NULL || !ON_GROUND(ch))
    {
        send_to_char("There is no ground here on which to scribe a lattice.\n", ch);
        return false;
    }

    // Check for existing lattice
    if (room_is_affected(ch->in_room, sn))
    {
        send_to_char("There is already a stone lattice here.\n", ch);
        return false;
    }

    // Echoes
    act("With two fingers you trace a lattice shape on the ground, infusing it with your will.", ch, NULL, NULL, TO_CHAR);
    act("With two fingers $n traces a lattice shape on the ground, murmuring softly.", ch, NULL, NULL, TO_ROOM);

    // Apply effect
    AFFECT_DATA af = {0};
    af.where    = TO_ROOM_AFF;
    af.type     = sn;
    af.level    = level;
    af.duration = 2 + (level / 17);
    affect_to_room(ch->in_room, &af);

    // Apply cooldown
    int skill(get_skill(ch, sn));
    af.where    = TO_AFFECTS;
    af.duration = 26 - (UMAX(0, skill - 70) / 5);
    affect_to_char(ch, &af);
    return true;
}

int determine_saltoftheearth_level(CHAR_DATA & ch, int sphere)
{
    int result(0);
    for (const AFFECT_DATA * paf(get_affect(&ch, gsn_saltoftheearth)); paf != NULL; paf = get_affect(&ch, gsn_saltoftheearth, paf))
    {
        if (paf->location == APPLY_NONE && paf->modifier == sphere)
            result += paf->level;
    }
    return result;
}

bool spell_saltoftheearth(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    // Verify this is a crystal
    OBJ_DATA * obj(static_cast<OBJ_DATA*>(vo));
    if (obj->pIndexData->vnum != OBJ_VNUM_MAGIC_CRYSTAL)
    {
        act("$p is not crystalline magic.", ch, obj, NULL, TO_CHAR);
        return false;
    }

    // Determine and verify the number of spheres involved
    unsigned int sphereCount(total_skill_spheres(obj->value[3]));
    if (sphereCount == 0)
    {
        act("$p does not contain appropriate magics for this working.", ch, obj, NULL, TO_CHAR);
        return false;
    }

    // Echoes
    act("You crush $p with a sharp word of power, drinking in its essence!", ch, obj, NULL, TO_CHAR);
    act("$n utters a sharp word of power, crushing $p into powder!", ch, obj, NULL, TO_ROOM);

    // Determine effects
    affect_strip(ch, sn);
    int spellLevel(obj->value[0] / sphereCount);
    spellLevel = UMAX(1, spellLevel);

    AFFECT_DATA af = {0};
    af.where    = TO_AFFECTS;
    af.type     = sn;
    af.level    = spellLevel;
    af.duration = 3 + spellLevel / 3;
    af.location = APPLY_NONE;

    for (unsigned int i(0); i < MAX_SKILL_SPHERE; ++i)
    {
        af.modifier = skill_table[obj->value[3]].spheres[i];
        switch (af.modifier)
        {
            case SPH_NONE: break;

            case SPH_WATER: 
                send_to_char("The released power carries with it a hint of the sea, soothing your body as you take it in.\n", ch); 
                affect_to_char(ch, &af);
                break;

            case SPH_EARTH: 
                send_to_char("The deep, steady power of the land fills your mind, bonding it to the earth.\n", ch); 
                affect_to_char(ch, &af);
                break;

            case SPH_VOID: 
                send_to_char("A dark crackle of energy flickers through your body in response.\n", ch);
                affect_to_char(ch, &af);
                break;

            case SPH_SPIRIT: 
                send_to_char("The ebbs and flows of magic flash briefly through your vision as you imbibe the power, before fading once more.\n", ch);
                affect_to_char(ch, &af);
                break;

            case SPH_AIR: 
                send_to_char("A gentle breeze seems to wash out from the crushed crystal, filling you with airy power.\n", ch);
                affect_to_char(ch, &af);
                break;

            case SPH_FIRE: 
                send_to_char("A wave of burning energy rages quickly through your body, lending you fiery strength.\n", ch);
                affect_to_char(ch, &af);
                break;

            default: 
                send_to_char("A sense of clarity fills your mind as it is boosted by the released energies.\n", ch);
                af.location = APPLY_MANA;
                af.modifier = number_range(level, level * 2);
                affect_to_char(ch, &af);
                break;
        }
    }

    extract_obj(obj);
    return true;
}

void check_bedrockroots(CHAR_DATA & ch)
{
    // Check whether bedrock roots applies
    if (ch.in_room == NULL || !ON_GROUND(&ch) || number_percent() > get_skill(&ch, gsn_bedrockroots))
        return;

    // Determine max bonus based on sector and level
    int maxBonus((ch.level / 2) * 4);
    switch (ch.in_room->sector_type)
    {
        case SECT_UNDERGROUND:  maxBonus += 150;    break;
        case SECT_MOUNTAIN:     maxBonus += 100;    break;
        case SECT_HILLS:        maxBonus += 75;     break;
        case SECT_SWAMP:                            break;
        default:                maxBonus += 25;     break;
    }

    // Determine actual bonus
    int bonus(maxBonus / 8);
    bonus = UMAX(bonus, 1);

    // Check for existing effect
    AFFECT_DATA * bedrock(get_affect(&ch, gsn_bedrockroots));
    if (bedrock == NULL)
    {
        // Add effect
        AFFECT_DATA af = {0};
        af.where    = TO_AFFECTS;
        af.type     = gsn_bedrockroots;
        af.level    = ch.level;
        af.duration = -1;
        af.location = APPLY_HIT;
        affect_to_char(&ch, &af);
        
        bedrock = get_affect(&ch, gsn_bedrockroots);
        if (bedrock == NULL)
        {
            bug("Could not get bedrockroots effect immediately after adding it", 0);
            return;
        }
    }
    
    // Check for maxed effect
    if (bedrock->modifier >= maxBonus)
        return;

    check_improve(&ch, NULL, gsn_bedrockroots, true, 12);
    bonus = UMIN(bonus, maxBonus - bedrock->modifier);
    bedrock->modifier += bonus;
    ch.max_hit += bonus;
    ch.hit += bonus;

    // Check for max
    if (bedrock->modifier >= maxBonus)
        send_to_char("You feel your bedrock roots settle deep within the earth, drawing fully upon its power.\n", &ch);
}

void shake_ground(CHAR_DATA * ch, ROOM_INDEX_DATA & room, int sn, int dam, bool ignoreGroup)
{
    act("The ground bucks and shakes, quaking beneath you!", room.people, NULL, NULL, TO_ALL);

    for (CHAR_DATA * victim(room.people); victim != NULL; victim = victim->next_in_room)
    {
        // Disqualify certain targets
        if (victim == ch || (ch != NULL && ignoreGroup && is_same_group(ch, victim)) 
        || is_flying(victim) || is_safe_spell(ch, victim, TRUE) 
        || (IS_NPC(victim) && (victim->pIndexData->vnum == MOB_VNUM_DRAKE || victim->pIndexData->vnum == MOB_VNUM_EARTH_ELEMENTAL))
        || is_stabilized(*victim) || (number_percent() <= (get_curr_stat(victim, STAT_DEX) - 10) * 3))
            continue;

        // Echoes
        send_to_char("You lose your balance, falling heavily to the ground!\n", victim);
        act("$n loses $s balance, falling heavily to the ground!", victim, NULL, NULL, TO_ROOM);

        // Deal damage
        if (dam > 0)
        {
            if (ch == NULL || ch->in_room != victim->in_room) sourcelessDamage(victim, "the heavy fall", dam, sn, DAM_BASH);
            else damage_old(ch, victim, dam, sn, DAM_BASH, true);
        }

        // Stun the target
        WAIT_STATE(victim, PULSE_VIOLENCE);
        stop_fighting(victim);
        if (!IS_NPC(victim))
            switch_position(victim, POS_SITTING);
    }

    // Echo to surrounding rooms
    for (unsigned int i(0); i < Direction::Max; ++i)
    {
        ROOM_INDEX_DATA * nextRoom(Direction::Adjacent(room, static_cast<Direction::Value>(i)));
        if (nextRoom != NULL && nextRoom->people != NULL && ON_GROUND(nextRoom->people))
            act("A tremor rumbles through the ground, rattling loose debris and sending up puffs of dust.", nextRoom->people, NULL, NULL, TO_ALL);
    }
}

static bool create_earth_maw(CHAR_DATA & victim, int level)
{
    // Echo about the earth maw
    send_to_char("The earth opens up beneath you, and you fall into the gaping maw!\n", &victim);
    act("A gaping maw opens up beneath $n, and $e falls within!", &victim, NULL, NULL, TO_ROOM);

    if (check_spirit_of_freedom(&victim))
    {
        send_to_char("You feel the spirit of freedom surge within you, and you reappear outside the earthmaw.\n\r", &victim);
        act("$n shimmers into existence outside the earthmaw.", &victim, NULL, NULL, TO_ROOM);
        return false;
    }

    ROOM_INDEX_DATA * prison(new_room_area(victim.in_room->area));

    copy_string(prison->name, "An Earthy Maw");
    copy_string(prison->description, "Thick soil and dirt surround you on every side, making movement in this\n\rplace nearly impossible, and breathing even more difficult.  A point\n\rof light shines in from above, the only physical means of escape from\n\rthis place.\n\r");
    prison->room_flags	= ROOM_NOGATE|ROOM_NOSUM_TO|ROOM_NOSUM_FROM|ROOM_NOWEATHER;
    prison->sector_type = SECT_UNDERGROUND;
    prison->heal_rate	= 0;
    prison->mana_rate	= 0;
    prison->move_rate	= 0;

    prison->exit[4] = new_exit();
    prison->exit[4]->u1.to_room = (victim.was_in_room ? victim.was_in_room : victim.in_room);

    AFFECT_DATA af = {0};
    af.where     = TO_ROOM;
    af.type      = gsn_earthmaw;
    af.level     = level;
    af.location  = APPLY_NONE;
    af.duration  = -1;
    affect_to_room(prison, &af);

    if (victim.in_room->vnum != 0)
        victim.was_in_room = victim.in_room;

	global_linked_move = TRUE;
    char_from_room(&victim);
    char_to_room(&victim, prison);
    do_look(&victim, "auto");
    return true;
} 

bool spell_cryofthebrokenlands(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    // Check for cooldown
    if (is_affected(ch, sn))
    {
        send_to_char("You have roused the earth's anger too recently to do so again.\n", ch);
        return false;
    }

    // Check for solid ground
    if (ch->in_room == NULL || !ON_GROUND(ch))
    {
        send_to_char("You cannot call to the earth from such a place!\n", ch);
        return false;
    }

    // Local echoes
    OBJ_DATA * weapon(get_eq_char(ch, WEAR_WIELD));
    if (weapon == NULL || weapon->item_type != ITEM_WEAPON || (weapon->value[0] != WEAPON_STAFF && weapon->value[0] != WEAPON_MACE))
    {
        act("You slam your hand on the ground, and a shockwave of energy ripples out from it!", ch, NULL, NULL, TO_CHAR);
        act("$n slams $s hand on the ground, and a shockwave of energy ripples out from it!", ch, NULL, NULL, TO_ROOM);
    }
    else
    {
        act("You strike the ground solidly with $p, sending a shockwave of energy rippling outwards!", ch, weapon, NULL, TO_CHAR);
        act("You strike the ground solidly with $p, sending a shockwave of energy rippling outwards!", ch, weapon, NULL, TO_CHAR);
    }

    // Spread in a radius up to 20 rooms out
    typedef std::set<ROOM_INDEX_DATA*> RoomSet;
    RoomSet rooms, nextRooms, doneRooms;
    rooms.insert(ch->in_room);
    for (unsigned int i(0); i < 20; ++i)
    {
        // Process each current room
        while (!rooms.empty())
        {
            // Move the first room to the completed set
            RoomSet::iterator iter(rooms.begin());
            ROOM_INDEX_DATA * room(*iter);
            rooms.erase(iter);
            doneRooms.insert(room);

            // Determine odds and damage from the room type and level
            int dam;
            int mawOdds;
            switch (room->sector_type)
            {
                case SECT_UNDERGROUND:  dam = 4 * level;    mawOdds = 16;   break;
                case SECT_MOUNTAIN:     dam = 3 * level;    mawOdds = 12;   break;
                case SECT_HILLS:        dam = 2 * level;    mawOdds = 8;    break;
                case SECT_DESERT:       dam = level / 2;    mawOdds = 20;   break;
                case SECT_SWAMP:        dam = (level / 4);  mawOdds = 12;   break;
                default:                dam = level;        mawOdds = 4;    break;
            }

            // Process the first room
            CHAR_DATA * victim_next;
            shake_ground(ch, *room, sn, dam + number_range(0, 10), false);
            for (CHAR_DATA * victim(room->people); victim != NULL; victim = victim_next)
            {
                victim_next = victim->next_in_room;

                // Ignore certain victims
                if (victim == ch || is_safe_spell(ch, victim, true) || is_flying(victim) || number_percent() > mawOdds
                || (IS_NPC(victim) && (victim->pIndexData->vnum == MOB_VNUM_DRAKE || victim->pIndexData->vnum == MOB_VNUM_EARTH_ELEMENTAL)))
                    continue;

                // Earth maw attempted, check for dex save
                if (number_percent() <= (get_curr_stat(victim, STAT_DEX) - 10) * 3)
                {
                    send_to_char("The earth opens up beneath you, but you leap clear!", victim);
                    act("The earth opens up beneath $n, but $e leaps clear in time!", victim, NULL, NULL, TO_ROOM);
                    continue;
                }

                // Actually send the victim into the maw
                if (create_earth_maw(*victim, level))
                    WAIT_STATE(victim, 2 * PULSE_VIOLENCE);
            }

            // Find the adjacent rooms
            for (unsigned int dir(0); dir < Direction::Max; ++dir)
            {
                ROOM_INDEX_DATA * nextRoom(Direction::Adjacent(*room, static_cast<Direction::Value>(dir)));
                if (nextRoom == NULL || !has_ground(*nextRoom) 
                || doneRooms.find(nextRoom) != doneRooms.end() || rooms.find(nextRoom) != rooms.end())
                    continue;

                nextRooms.insert(nextRoom);
            }
        }

        // Swap for the next go-round
        std::swap(rooms, nextRooms);
    }

    // Apply the cooldown
    AFFECT_DATA af = {0};
    af.where    = TO_AREA;
    af.type     = sn;
    af.level    = level;
    af.duration = 8;
    affect_to_char(ch, &af);
    return true;
}

bool spell_quake(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    // Verify the room
    if (ch->in_room == NULL || !ON_GROUND(ch))
    {
        send_to_char("There is no ground here to quake!\n", ch);
        return false;
    }

    // Calculate damage; wakenstone offers a bonus
    int dam(dice(level, 2));
    if (number_percent() <= get_skill(ch, gsn_wakenedstone))
    {
        check_improve(ch, NULL, gsn_wakenedstone, 12, true);
        dam += (level / 3);
    }

    // Shake the ground
    shake_ground(ch, *ch->in_room, sn, dam, true);
    return true;
}

bool spell_shakestride(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    // Check for cooldown or already shaking
    AFFECT_DATA * paf(get_affect(ch, sn));
    if (paf != NULL)
    {
        if (paf->modifier == 0) send_to_char("Your steps already shake the earth!\n", ch);
        else send_to_char("You are not ready to empower your steps again yet.\n", ch);
        return false;
    }

    // Echoes
    send_to_char("You fill your feet with the earth's power, readying your steps to shake the lands.\n", ch);

    // Apply effect
    AFFECT_DATA af = {0};
    af.where    = TO_AFFECTS;
    af.type     = sn;
    af.level    = level;
    af.duration = 3 + (level / 17);
    affect_to_char(ch, &af);
    return true;
}

bool spell_stonehaven(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    // Check for cooldown
    if (is_affected(ch, sn))
    {
        send_to_char("You are not prepared to claim another stonehaven yet.\n", ch);
        return false;
    }

    // Make sure the room is of acceptable type
    if (ch->in_room == NULL || !ON_GROUND(ch))
    {
        send_to_char("There is no ground here to claim as a haven!\n", ch);
        return false;
    }

    // Check for already present
    if (room_is_affected(ch->in_room, sn))
    {
        send_to_char("This place has already been claimed as a stonehaven.\n", ch);
        return false;
    }

    // Echoes
    act("You strike the earth with both fists, plunging your arms through the ground up to the elbows!", ch, NULL, NULL, TO_CHAR);
    act("$n strikes the earth with both fists, plunging $s arms through the ground up to the elbows!", ch, NULL, NULL, TO_ROOM);

    // Set up the handler
    struct CallbackHandler
    {
        static bool HandleMove(CHAR_DATA * ch, ROOM_INDEX_DATA *, EchoAffect *, void *) {HandleCancel(ch); return true;}
        static bool HandlePositionChange(CHAR_DATA * ch, int, EchoAffect *, void *) {HandleCancel(ch); return true;}
        static bool HandleCast(CHAR_DATA * ch, int, int, void *, int, EchoAffect *, void *) {HandleCancel(ch); return true;}
        static bool Finish(CHAR_DATA * ch, EchoAffect *, void * tag)
        {
            // Add the room effect
            AFFECT_DATA af = {0};
            af.where    = TO_ROOM_AFF;
            af.type     = gsn_stonehaven;
            af.level    = reinterpret_cast<int>(tag);
            af.duration = af.level / 2;
            affect_to_room(ch->in_room, &af);

            // Apply a cooldown
            int skill(get_skill(ch, gsn_stonehaven));
            af.where    = TO_AFFECTS;
            af.duration = 30 - (UMAX(0, skill - 70) / 6);
            affect_to_char(ch, &af);

            // Stop fliers
            for (CHAR_DATA * vch(ch->in_room->people); vch != NULL; vch = vch->next_in_room)
            {
                if (is_flying(vch) && !check_spirit_of_freedom(vch))
                {
                    stop_flying(*vch);
                    send_to_char("You are pulled to the ground by the earthen forces!\n", vch);
                    act("$n is pulled to the ground, ceasing to fly.", vch, NULL, NULL, TO_ROOM);
                }
            }

            return false;
        }
         
        private:   
            static void HandleCancel(CHAR_DATA * ch)
            {
                send_to_char("You pull your arms from the earth, letting the growing power diminish once more.\n", ch);
                act("$n pulls $s arms from the earth, and the sense of building power fades.", ch, NULL, NULL, TO_ROOM);
            }
    };

    // Prepare the echoAffect
    EchoAffect * echoAff(new EchoAffect(1));
    echoAff->SetPositionCallback(&CallbackHandler::HandlePositionChange);
    echoAff->SetMoveCallback(&CallbackHandler::HandleMove);
    echoAff->SetCastCallback(&CallbackHandler::HandleCast);

    // Add in lines
    echoAff->AddLine(&CallbackHandler::Finish,
                    "You feel the power of this place surge up your arms as you claim it as a stonehaven!",
                    "The ground hums with power, sending vibrations up through $n's arms!");

    // Finish apply the effect
    echoAff->SetTag(reinterpret_cast<void*>(level));
    EchoAffect::ApplyToChar(ch, echoAff);
    return true;
}

bool spell_conduitofstonesong(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    // Check for drake
    CHAR_DATA * drake(NULL);
    if (ch->pet != NULL && ch->pet->in_room == ch->in_room && ch->in_room != NULL
    && IS_NPC(ch->pet) && ch->pet->pIndexData->vnum == MOB_VNUM_DRAKE)
        drake = ch->pet;

    // Check whether effect is already present
    AFFECT_DATA * paf(get_affect(ch, sn));
    if (paf == NULL)
    {
        // Not already present, so start it up
        AFFECT_DATA af = {0};
        af.where    = TO_AFFECTS;
        af.type     = sn;
        af.level    = level;
        af.duration = -1;
        affect_to_char(ch, &af);

        // Echoes
        send_to_char("You immerse your mind in the deep, subtle rhythm of the earth, letting its song pour forth through you!\n", ch);
        act("A deep, sonorous hum begins to resonate within $n.", ch, NULL, NULL, TO_ROOM);

        // Echoes if drake is present
        if (drake != NULL)
            act("$n seems to swell up, $s claws lengthening and stony hide thickening!", drake, NULL, NULL, TO_ROOM);
        return true;
    }

    // Strip the effect
    affect_remove(ch, paf);
    send_to_char("You withdraw your mind from the earth, and the stonesong dies within you.\n", ch);
    act("The deep humming sound resonating about $n slowly ceases.", ch, NULL, NULL, TO_ROOM);

    // Echoes if drake is present
    if (drake != NULL)
        act("$n seems to shrink slightly, $s claws and stony plating diminishing.", drake, NULL, NULL, TO_ROOM);
    return true;
}

void do_wakenstone(CHAR_DATA * ch, char * argument)
{
    // Check for skill
    int skill(get_skill(ch, gsn_wakenedstone));
    if (skill <= 0)
    {
        send_to_char("Huh?\n", ch);
        return;
    }

    // Farm out the rest to the drakes class
    Drakes::WakenStone(*ch, argument, skill);
}

bool spell_gravenmind(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    static const int MaxHPLoss(200);

    // Check for cooldown
    if (is_affected(ch, sn))
    {
        send_to_char("You are already tapping your body for the sake of your mind.\n", ch);
        return false;
    }

    // Get the argument
    int hpToTap;
    if (target_name[0] == '\0') 
        hpToTap = UMIN(MaxHPLoss, ch->max_hit / 10);
    else if (is_number(target_name))
    {
        hpToTap = atoi(target_name);
        if (hpToTap > MaxHPLoss || hpToTap >= ch->max_hit)
        {
            send_to_char("You cannot tap your body so severely.\n", ch);
            return false;
        }
    }
    else
    {
        send_to_char("Please specify a number or leave blank.\n", ch);
        return false;
    }

    // Sanity-check
    if (hpToTap <= 0)
    {
        send_to_char("You must tap a least a little of yourself if you seek the graven mind.\n", ch);
        return false;
    }

    // Prepare the new effect
    AFFECT_DATA af = {0};
    af.where    = TO_AFFECTS;
    af.type     = sn;
    af.level    = level;
    af.duration = 30;
    af.location = APPLY_HIT;
    af.modifier = -hpToTap;
    affect_to_char(ch, &af);

    af.location = APPLY_MANA;
    af.modifier = hpToTap * 2;
    affect_to_char(ch, &af);

    ch->mana += af.modifier;
    ch->hit = UMIN(ch->hit, ch->max_hit);

    send_to_char("You draw deeply of your own strength, engraving lines of power into your mind!\n", ch);
    return true;
}

bool spell_markofloam(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    // Check for an order fount here
    if (ch->in_room == NULL || !Weave::HasWeave(*ch->in_room) 
    || !ch->in_room->ley_group->HasFount() || ch->in_room->ley_group->FountOrderPower() <= 0)
    {
        send_to_char("You do not sense a nexus of orderly power in this place.\n", ch);
        return false;
    }

    // Check whether already attuned to this caster
    if (ch->in_room->ley_group->AttunedID() == ch->id)
    {
        send_to_char("You have already laid down a mark of dominion in this place.\n", ch);
        return false;
    }

    // Set up the spell
    struct CallbackHandler
    {
        static bool HandleMove(CHAR_DATA * ch, ROOM_INDEX_DATA *, EchoAffect *, void *)
        {
            HandleCancel(ch);
            return true;
        }

        static bool HandlePositionChange(CHAR_DATA * ch, int newPos, EchoAffect *, void *)
        {
            if (newPos == POS_STANDING)
                return false;

            HandleCancel(ch);
            return true;
        }

        static bool FinishSpell(CHAR_DATA * ch, EchoAffect *, void *)
        {
            ch->in_room->ley_group->SetAttunedID(ch->id);
            Weave::UpdateAttunements();
            Weave::SaveWeave();
            return false;
        }

        private:
            static void HandleCancel(CHAR_DATA * ch)
            {
                send_to_char("You let slip your focus on the mark, and the building energies recede.\n", ch);
            }
    };

    // Handle echos
    act("You focus your mind, calling up the image of the mark of Dominion.", ch, NULL, NULL, TO_CHAR);
    act("$n grows still, lost in concentration.", ch, NULL, NULL, TO_ROOM);

    // Prepare effect
    EchoAffect * echoAff(new EchoAffect(1));
    echoAff->SetMoveCallback(&CallbackHandler::HandleMove);
    echoAff->SetPositionCallback(&CallbackHandler::HandlePositionChange);

    echoAff->AddLine(NULL,
                    "Holding the mark in your mind, you stretch out a finger and trace its rigid lines before you.",
                    "$n traces a symbol with a single finger, held out in front of $m.");

    echoAff->AddLine(NULL,
                    "With an act of will you thread the ordered energy of this place through the mark, attuning it to your resonance!",
                    "A faint, otherworldly hum starts up around $n.");

    echoAff->AddLine(&CallbackHandler::FinishSpell,
                    "You seal the spell with a final word, feeling the power spread throughout your being.",
                    "As the hum slowly dies away, $n seems somehow sturdier.");

    EchoAffect::ApplyToChar(ch, echoAff);
    return true;
}

void performAdamantineCast(CHAR_DATA * ch, CHAR_DATA * victim, int sn, int level, const char * currTargetName)
{
    struct Info
    {
        std::string originalTargetName;
        int victimID;
        int spellSN;
        int level;
    };

    struct CallbackHandler
    {
        static bool HandlePositionChange(CHAR_DATA * ch, int newPos, EchoAffect *, void * tag)
        {
            if (newPos < POS_FIGHTING)
            {
                CancelSpell(ch, tag);
                return true;
            }

            return false;
        }

        static bool HandleMove(CHAR_DATA * ch, ROOM_INDEX_DATA * room, EchoAffect *, void * tag)
        {
            CancelSpell(ch, tag);
            return true;
        }

        static bool HandleCast(CHAR_DATA * ch, int, int, void *, int, EchoAffect *, void * tag)
        {
            CancelSpell(ch, tag);
            return true;
        }

        static bool ContinueSpell(CHAR_DATA * ch, EchoAffect *, void * tag)
        {
            return (LookupVictim(ch, tag) == NULL);
        }

        static bool FinishSpell(CHAR_DATA * ch, EchoAffect *, void * tag)
        {
            // Get the victim
            CHAR_DATA * victim(LookupVictim(ch, tag));
            if (victim == NULL)
                return false;
           
            // Get the adamantine effect 
            AFFECT_DATA * paf(get_affect(ch, gsn_adamantineinvocation));
            if (paf == NULL)
            {
                // This is arguably a bug; it should really not happen except with astrip
                send_to_char("The power of the earth dissipates within you.\n", ch);
                return false;
            }

            // Cast the spell
            Info * info(static_cast<Info*>(tag));
            paf->modifier = info->level;
            check_improve(ch, victim, gsn_adamantineinvocation, true, 2);
            send_to_char("You unleash the spell, reinforced with the power of the earth itself!\n", ch);
            target_name = str_dup(info->originalTargetName.c_str());
            (*skill_table[info->spellSN].spell_fun)(info->spellSN, info->level, ch, victim, TARGET_CHAR);
            free_string(target_name);
            delete info;
            return false;
        }

        private:
            static void CancelSpell(CHAR_DATA * ch, void * tag)
            {
                send_to_char("You abandon your efforts to summon the forces of earth.\n", ch);
                delete static_cast<Info*>(tag);
            }

            static CHAR_DATA * LookupVictim(CHAR_DATA * ch, void * tag)
            {
                // Sanity-check
                Info * info(static_cast<Info*>(tag));
                if (ch->in_room != NULL)
                {
                    // Find the victim in the room
                    for (CHAR_DATA * victim(ch->in_room->people); victim != NULL; victim = victim->next_in_room)
                    {
                        if (victim->id == info->victimID)
                            return victim;
                    }
                }

                // Clean it up
                send_to_char("With your target gone, you abandon your efforts to summon the forces of earth.\n", ch);
                delete info;
                return NULL;
            }
    };

    // Check for rune of adamantine
    int runeCount(static_cast<int>(Runes::InvokedCount(*ch, Rune::Adamantine)));

    // Add a cooldown to the caster
    AFFECT_DATA af = {0};
    af.where    = TO_AFFECTS;
    af.type     = gsn_adamantineinvocation;
    af.level    = ch->level;
    af.modifier = 0;
    af.duration = UMAX(6, (18 - (runeCount * 6)));
    affect_to_char(ch, &af);

    // Prepare the info tag
    Info * info(new Info);
    info->originalTargetName = currTargetName;
    info->victimID = victim->id;
    info->spellSN = sn;
    info->level = level;

    // Prepare the effect
    EchoAffect * echoAff(new EchoAffect(1));
    echoAff->SetTag(info);
    echoAff->SetPositionCallback(&CallbackHandler::HandlePositionChange);
    echoAff->SetMoveCallback(&CallbackHandler::HandleMove);
    echoAff->SetCastCallback(&CallbackHandler::HandleCast);

    // Add in lines as needed
    if (runeCount <= 0)
        echoAff->AddLine(&CallbackHandler::ContinueSpell, "The adamantine power of stone begins to permeate your will!");

    echoAff->AddLine(&CallbackHandler::FinishSpell, "");
    EchoAffect::ApplyToChar(ch, echoAff);

    // Echoes
    send_to_char("You focus your will on the earth, and feel it stir in response.\n", ch);
    act("A slow, steady hum starts up in the earth at $n's feet.", ch, NULL, NULL, TO_ROOM);
}

void do_adamantineinvocation(CHAR_DATA * ch, char * argument)
{
    // Basic skill check
    if (get_skill(ch, gsn_adamantineinvocation) <= 0)
    {
        send_to_char("Huh?\n", ch);
        return;
    }

    // Location check
    if (ch->in_room == NULL || !ON_GROUND(ch))
    {
        send_to_char("You cannot tap enough of the earth's power here.\n", ch);
        return;
    }

    // Cooldown check
    if (is_affected(ch, gsn_adamantineinvocation))
    {
        send_to_char("You are not ready to call forth the forces of earth again just yet.\n", ch);
        return;
    }

    perform_spellcasting(ch, argument, true);
}

static void do_carve_syntax(CHAR_DATA * ch)
{
    send_to_char("Syntax:\n", ch);
    send_to_char("{Wcarve stone <object> <name>{x: used to carve a stone object, changing its name as specified\n", ch);
    send_to_char("{Wcarve staff|club <object>{x: used to carve a staff or club from a stone object\n", ch);
    send_to_char("{Wcarve rune <object> <rune>{x: used to carve a rune into an object\n\n", ch);
}

static void do_rune_syntax(CHAR_DATA * ch)
{
    send_to_char("Syntax:\n", ch);
    send_to_char("{Wrune list{x: used to list known, invoked, and invokable runes\n", ch);
    send_to_char("{Wrune invoke <object|#>{x: used to invoke the specified rune\n", ch);
    send_to_char("{Wrune revoke <object|#>{x: used to cease invoking the specified rune\n", ch);

    if (IS_IMMORTAL(ch))
    {
        send_to_char("\nImmortal-only:\n", ch);
        send_to_char("{Wrune list <name>{x: used like rune list but displays for the specified PC\n", ch);
    }
}

static void do_carve_fail_destroy(CHAR_DATA * ch, OBJ_DATA * obj)
{
    act("You begin to carve $p with earthen magics, but lose focus!", ch, obj, NULL, TO_CHAR);

    // Check for sufficient wisdom to stop
    if (number_percent() <= ((get_curr_stat(ch, STAT_WIS) - 10) * 4))
    {
        act("Realizing your mistake in time, you wisely stop carving before you do any real damage.", ch, obj, NULL, TO_CHAR);
        act("$n begins carving $p, but stops after marking just a few scratches.", ch, obj, NULL, TO_ROOM);
        return;
    }

    // Did not stop in time, destroy the object
    act("You struggle to regain control of the energy, but cannot stop it; $p crumbles to dust in your hands!", ch, obj, NULL, TO_CHAR);
    act("$n begins carving $p, but it suddenly crumbles to dust in $s hands!", ch, obj, NULL, TO_ROOM);
    extract_obj(obj);
}

static OBJ_DATA * do_carve_make_base_obj(CHAR_DATA * ch, OBJ_DATA * obj, const char * shortDesc)
{
    // Make the base obj
    OBJ_DATA * result(create_object(get_obj_index(OBJ_VNUM_STONECRAFT), 0));
    if (result == NULL)
    {
        bug("Stonecraft failed due to bad obj creation", 0);
        send_to_char("An error has occurred, please contact the gods.\n", ch);
        return NULL;
    }

    // Send echoes
    std::ostringstream mess;
    mess <<"You concentrate earthen magics on $p, carving it carefully into " << shortDesc << "!";
    act(mess.str().c_str(), ch, obj, NULL, TO_CHAR);

    mess.str("");
    mess << "$n concentrates on $p, carving it carefully into " << shortDesc << "!";
    act(mess.str().c_str(), ch, obj, NULL, TO_ROOM);

    // Make the name and long (the name gets an additional value to avoid cheesy triggering of give_progs)
    std::string name(shortDesc);
    name += " obj_stonecraft";

    std::string longDesc(shortDesc);
    longDesc += " is here.";
    longDesc[0] = UPPER(longDesc[0]);

    // Set the string values
    setName(*result, name.c_str());
    copy_string(result->short_descr, shortDesc);
    copy_string(result->description, longDesc.c_str());
    
    // Set the common numerical values
    result->material = obj->material;
    result->level = UMIN(ch->level, obj->level);
    result->weight = UMAX(1, obj->weight - 15);
    result->size = obj->size;

    // The object is definitely going to the char at this point, so destroy the original and give this one
    extract_obj(obj);
    obj_to_char(result, ch);
    return result;
}

static bool do_carve_stone(CHAR_DATA * ch, OBJ_DATA * obj, bool success, const char * argument)
{
    // Check for success
    if (!success)
    {
        do_carve_fail_destroy(ch, obj);
        return true;
    }

    // Load up the base object; also handles echoes
    obj = do_carve_make_base_obj(ch, obj, argument);
    if (obj == NULL)
        return false;

    // Adjust the wear flags and cost
    SET_BIT(obj->wear_flags, ITEM_HOLD|ITEM_TAKE);

    if (number_percent() <= get_skill(ch, gsn_stoneshape))
    {
        check_improve(ch, NULL, gsn_stoneshape, true, 3);
        obj->cost = dice(obj->level, 51);
    }
    else
    {
        check_improve(ch, NULL, gsn_stoneshape, false, 3);
        obj->cost = dice(obj->level, 31);
    }

    // Drop the ch into the string editor to desc the object
    send_to_char("With care, you add the following detail to it:\n", ch);
    EXTRA_DESCR_DATA * extraDesc(new_extra_descr());
    extraDesc->keyword  = str_dup(obj->name);
    extraDesc->next     = obj->extra_descr;
    obj->extra_descr    = extraDesc;
    string_append(ch, &extraDesc->description);
    return true;
}

static bool do_carve_weapon(CHAR_DATA * ch, OBJ_DATA * obj, bool success, int weaponType, int flags, const char * weaponName)
{
    // Check for success
    if (!success)
    {
        do_carve_fail_destroy(ch, obj);
        return true;
    }

    // Check for stoneshape
    bool stoneshaped(number_percent() <= get_skill(ch, gsn_stoneshape));
    check_improve(ch, NULL, gsn_stoneshape, stoneshaped, 3);

    // Build up the short desc
    std::ostringstream shortDesc;
    if (!stoneshaped) shortDesc << "crude ";
    shortDesc << material_table[obj->material].name << " " << weaponName;
    std::string shortDescStr(shortDesc.str());
    std::string finalShort(indefiniteArticleFor(shortDescStr[0]));
    finalShort += ' ';
    finalShort += shortDescStr;

    // Load up the base object; also handles echoes
    obj = do_carve_make_base_obj(ch, obj, finalShort.c_str());
    if (obj == NULL)
        return false;

    // Charge mana and lag
    expend_mana(ch, skill_table[gsn_stonecraft].min_mana);
    WAIT_STATE(ch, skill_table[gsn_stonecraft].beats);

    // Adjust the wear flags and other weapon attributes based on level and stoneshape
    SET_BIT(obj->wear_flags, ITEM_WIELD|ITEM_TAKE);
    obj->item_type = ITEM_WEAPON;
    copy_string(obj->obj_str, "crush");
    obj->value[0] = weaponType;
    obj->value[1] = (obj->level / 17) + (stoneshaped ? 9 : 8);
    obj->value[2] = 3;
    obj->value[3] = DAM_BASH;
    obj->value[4] = flags;
    return true;
}

void do_carve(CHAR_DATA * ch, char * argument)
{
    enum Type {Stone, Staff, Club, Rune};

    // Check skill
    if (!IS_IMMORTAL(ch) && get_skill(ch, gsn_stonecraft) <= 0)
    {
        send_to_char("Huh?\n", ch);
        return;
    }

    // Get the next argument
    char arg[MAX_INPUT_LENGTH];
    argument = one_argument(argument, arg);

    // Determine the command type
    Type type;
    if (!str_prefix(arg, "stone")) type = Stone;
    else if (!str_prefix(arg, "staff")) type = Staff;
    else if (!str_prefix(arg, "club")) type = Club;
    else if (!str_prefix(arg, "rune")) type = Rune;
    else {do_carve_syntax(ch); return;}

    // Check for cooldown
    for (AFFECT_DATA * paf(get_affect(ch, gsn_stonecraft)); paf != NULL; paf = get_affect(ch, gsn_stonecraft, paf))
    {
        if (paf->location == APPLY_NONE)
        {
            send_to_char("You are still tired from your last carving.\n", ch);
            return;
        }
    }

    // Get the next argument and use it to look up the target object
    argument = one_argument(argument, arg);
    OBJ_DATA * obj(get_obj_carry(ch, arg, ch));
    if (obj == NULL)
    {
        send_to_char("You are carrying nothing by that name.\n", ch);
        return;
    }

    // Check for a stone material, or metal if type rune
    int skillSN(-1);
    if (material_table[obj->material].stone) skillSN = gsn_stonecraft;
    else if (type == Rune && material_table[obj->material].metal) skillSN = gsn_forgemaster;

    // Get the skill and verify material
    int skill(skillSN >= 0 ? get_skill(ch, skillSN) : 0);
    if (skill <= 0)
    {
        send_to_char("You do not know how to carve that material.\n", ch);
        return;
    }

    if (type != Rune)
    {
        // Check for minimum size
        if (obj->size < SIZE_SMALL)
        {
            act("$p is too small to properly carve.", ch, obj, NULL, TO_CHAR);
            return;
        }

        // Check for minimum weight
        if (obj->weight < 20)
        {
            act("$p is not massive enough to properly carve.", ch, obj, NULL, TO_CHAR);
            return;
        }
    }

    // Check for nodestroy
    if (IS_SET(obj->extra_flags[0], ITEM_NODESTROY))
    {
        act("$p is far too tough to carve.", ch, obj, NULL, TO_CHAR);
        return;
    }
    
    // Determine success and dispatch according to type
    bool success(number_percent() <= skill);
    bool occurred(false);
    switch (type)
    {
        case Stone: occurred = do_carve_stone(ch, obj, success, argument); break;
        case Staff: occurred = do_carve_weapon(ch, obj, success, WEAPON_STAFF, WEAPON_TWO_HANDS, "staff"); break; 
        case Club:  occurred = do_carve_weapon(ch, obj, success, WEAPON_MACE, 0, "club"); break;
        case Rune:  occurred = Runes::CarveRune(*ch, *obj, success, argument); break;
    }

    if (!occurred)
        return;
   
    // Handle skill improvements and apply a cooldown
    check_improve(ch, NULL, skillSN, success, 3);
    AFFECT_DATA af = {0};
    af.where    = TO_AFFECTS;
    af.type     = gsn_stonecraft;
    af.level    = ch->level;
    af.location = APPLY_NONE;
    af.duration = 40 - UMIN(10, (ch->level / 5));
    affect_to_char(ch, &af);
}

static void do_rune_list(CHAR_DATA * ch, const char * argument)
{
    // Determine who the actual crafter for whom to list runes is
    CHAR_DATA * crafter(ch);
    if (IS_IMMORTAL(ch) && argument[0] != '\0')
    {
        crafter = get_char_world(ch, argument);
        if (crafter == NULL)
        {
            send_to_char("You see no one by that name in the world.\n", ch);
            return;
        }
    }

    // Display according to the argument
    std::string text(Runes::ListRunes(*crafter));
    if (text.empty())
    {
        if (ch != crafter) act("$N does not know any runes.", ch, NULL, crafter, TO_CHAR);
        else send_to_char("You are not studied in the carving of any earthen runes.\n", ch);
        return;
    }

    send_to_char(text.c_str(), ch);
}

void do_rune(CHAR_DATA * ch, char * argument)
{
    // Check skill
    if (!IS_IMMORTAL(ch) && get_skill(ch, gsn_stonecraft) <= 0)
    {
        send_to_char("Huh?\n", ch);
        return;
    }

    // Get the next argument
    char arg[MAX_INPUT_LENGTH];
    argument = one_argument(argument, arg);

    // Dispatch according to the argument
    if (!str_prefix(arg, "list")) do_rune_list(ch, argument);
    else if (!str_prefix(arg, "invoke")) Runes::InvokeRune(*ch, argument);
    else if (!str_prefix(arg, "revoke")) Runes::RevokeRune(*ch, argument);
    else do_rune_syntax(ch);
}

bool spell_bindweapon( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    OBJ_DATA *wield;
    AFFECT_DATA af;
    af.valid = TRUE;
    af.point = NULL;

    if ((wield = get_eq_char(victim, WEAR_WIELD)) == NULL)
    {
	if (victim == ch)
	    send_to_char("You aren't holding a weapon.\n\r", ch);
	else
	    send_to_char("They aren't holding any weapons.\n\r", ch);
	return FALSE;
    }

    if (obj_is_affected(wield,sn))
    {
	if (victim == ch)
	    send_to_char("Your weapon is already bound to your grasp.\n\r",ch);
	else
	    send_to_char("Their weapon is already bound to their grasp.\n\r",ch);
        return FALSE;
    }

    af.where	 = TO_OBJECT;
    af.type	 = sn;
    af.level	 = level;
    af.duration  = level/3;
    af.location  = APPLY_NONE;
    af.modifier  = 0;
    af.bitvector = 0;
    affect_to_obj(wield, &af);

    if ((wield = get_eq_char(victim, WEAR_DUAL_WIELD)) != NULL)
	affect_to_obj(wield, &af);

    if (victim == ch)
	send_to_char("Calling upon the powers of earth, you bind your weapon in your grip.\n\r", ch);
    else
    {
	act("Calling upon the powers of earth, you bind $N's weapons in $S grasp.", ch, NULL, victim, TO_CHAR);
	send_to_char("You feel your weapons bind themselves in your grasp.\n\r", victim);
    }

    return TRUE;
}
	
bool spell_brittleform( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;

    if (is_affected(victim, sn))
    {
        send_to_char("They're already brittle.\n\r", ch);
        return FALSE;
    }

    if (!is_same_group(ch, victim) && saves_spell(level, ch, victim, DAM_OTHER))
	{
	    act("$n's skin begins to turn brittle, but returns to normal.", victim, NULL, NULL, TO_ROOM);
	    act("Your skin begins turning brittle, but returns to normal.", victim, NULL, NULL, TO_CHAR);
	    return TRUE;
	}

    AFFECT_DATA af = {0};
    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = 2 + level / 8;
    af.location  = APPLY_RESIST_BASH;
    af.modifier  = -20;
    affect_to_char( victim, &af );

    af.location = APPLY_RESIST_SLASH;
    af.modifier  = 20;
    affect_to_char( victim, &af );
    act( "$n's skin takes on a slightly glassy look.", victim, NULL, NULL, TO_ROOM);
    act( "Your skin takes on a slightly glassy look.", victim, NULL, NULL, TO_CHAR);
    return TRUE;
}


bool spell_calluponearth( int sn, int level, CHAR_DATA *ch, void *vo,int target)
{
    if (ch->in_room == NULL || ch->in_room->sector_type != SECT_UNDERGROUND)
    {
        send_to_char("You are not close enough to the heart of the earth to call upon it.\n\r", ch);
        return false;
    }

    // Calculate healing
    int heal(dice(5, 10) + level - 6);
    if (number_percent() <= get_skill(ch, gsn_wakenedstone))
    {
        check_improve(ch, NULL, gsn_wakenedstone, true, 12);
        heal += level / 10;
    }

    ch->hit = UMIN(ch->hit + heal, ch->max_hit);
    update_pos(ch);
    send_to_char("You feel better!\n", ch);
    return true;
}


bool spell_crystalizemagic( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if ( is_affected( ch, sn ) )
    {
        send_to_char("You cannot absorb magic again so soon.\n\r", ch);
        return FALSE;
    }

    af.where     = TO_PAFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = level/8;
    af.modifier  = 0;
    af.location  = 0;
    af.bitvector = AFF_CRYSTALIZE_MAGIC;
    affect_to_char( ch, &af );
    send_to_char( "The air around you shimmers with crystalline energy.\n\r", ch );

    af.bitvector = 0;
    af.modifier  = 1;
    af.duration  = 20 - (level/5);
    affect_to_char(ch, &af);

    return TRUE;
}


bool spell_density( int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;

    if (is_affected(victim, sn))
    {
        if (ch == victim) send_to_char("You are already dense.\n", ch);
        else act("$N is already dense.", ch, NULL, victim, TO_CHAR);
        return false;
    }

    if (victim->size < SIZE_MEDIUM)
    {
        if (victim == ch) send_to_char("You are too small to be affected.\n", ch);
        else act("$N is too small to be affected.", ch, NULL, victim, TO_CHAR);
        return false;
    }

    if (ch != victim && saves_spell(level,ch,victim,DAM_OTHER))
    {
        act("$E resists your spell.", ch, NULL, victim, TO_CHAR);
        return true;
    }

    AFFECT_DATA af = {0};
    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = (level * (check_durablemagics(*ch) ? 2 : 1)) / 8;
    affect_to_char( victim, &af );

    send_to_char( "You feel heavy!\n\r", victim );
    act("$n appears to be having trouble keeping upright.",victim,NULL,NULL,TO_ROOM);
    return TRUE;
}

bool spell_devotion( int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    OBJ_DATA *vObj = (OBJ_DATA *) vo;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if (is_affected(ch, sn))
    {
	send_to_char("You are not prepared to devote yourself to another weapon yet.\n\r", ch);
	return FALSE;
    }

    if (vObj->item_type != ITEM_WEAPON)
    {
        send_to_char("You may only devote yourself to weapons.\n\r", ch);
        return FALSE;
    }

    if (obj_is_affected(vObj, gsn_devotion))
    {
	if (get_high_modifier(vObj->affected, gsn_devotion) == ch->id)
	{
	    send_to_char("You are already devoted to that weapon.\n\r", ch);
	    return FALSE;
	}
	else
	    object_affect_strip(vObj, gsn_devotion);
    }

    af.where	 = TO_AFFECTS;
    af.type	 = sn;
    af.level	 = level;
    af.modifier  = 0;
    af.duration  = 100;
    af.location  = APPLY_NONE;
    af.bitvector = 0;
    affect_to_char(ch, &af);

    af.duration  = -1;
    af.where	 = TO_OBJECT;
    af.modifier  = ch->id;
    affect_to_obj(vObj, &af);

    act("You channel tendrils of earth magic around $p, devoting yourself to its use.", ch, vObj, NULL, TO_CHAR);

    return TRUE;
}

bool spell_diamondskin( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if (is_affected(ch, gsn_diamondskin))
    {
        send_to_char("You don't feel able to harden your skin again yet.\n\r", ch);
        return FALSE;
    }

    af.where     = TO_IMMUNE;
    af.type      = sn;
    af.level     = level;
    af.duration  = 1;
    af.modifier  = 0;
    af.location  = 0;
    af.bitvector = IMM_WEAPON;
    affect_to_char( ch, &af );
    send_to_char( "Your skin turns hard as diamond.\n\r", ch );
    act("$n's skin turns diamond hard.", ch, NULL, NULL, TO_ROOM);

    af.where     = TO_AFFECTS;
    af.duration  = 10;
    af.location  = 0;
    af.bitvector = 0;
    affect_to_char(ch, &af);

    return TRUE;
}

bool spell_dispelillusions( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
int direction;
EXIT_DATA *pexit;
CHAR_DATA *vch, *vch_next;

        if (!ch->in_room)
                return FALSE;

        if (ch->in_room->sector_type == SECT_AIR)
                {
                send_to_char("You cannot call upon the forces of earth in the air!\n\r", ch);
                return FALSE;
                }

        act("You call upon the forces of earth to stabilize your surroundings!", ch, NULL, NULL, TO_CHAR);
        act("$n calls upon the forces of earth to stabilize $s surroundings!", ch, NULL, NULL, TO_ROOM);

        for (direction = 0; direction < 6; direction++)
        {
        if ((pexit = ch->in_room->exit[direction]) == NULL)
                continue;
        if (ch->in_room->exit[direction]->exit_info & EX_ILLUSION)
                {
                free_exit(ch->in_room->exit[direction]);
                ch->in_room->exit[direction] = NULL;
                act("An illusionary exit disappears.", ch, NULL, NULL, TO_CHAR);
                act("An illusionary exit disappears.", ch, NULL, NULL, TO_ROOM);
                }
        }

        for (vch = ch->in_room->people; vch != NULL; vch = vch_next)
        {
            vch_next = vch->next_in_room;
            if (IS_NPC(vch) && (vch->act & ACT_ILLUSION) && !saves_spell(level, ch, vch, DAM_OTHER))
            {
                act("$n is torn apart by the magic of earth, and the ribbons of illusion are pulled into the ground!", vch, NULL, NULL, TO_ROOM);
                extract_char(vch, TRUE);
            }
        }
    return TRUE;
}

bool spell_earthbind(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *vch = (CHAR_DATA *) vo;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if (is_affected(vch, gsn_earthbind))
    {
        send_to_char("They are already earth-bound.\n\r", ch);
        return FALSE;
    }

    if (check_spirit_of_freedom(vch))
    {
	send_to_char("You feel the spirit of freedom surge within you.\n\r", vch);
	act("$n glows brightly for a moment.", vch, NULL, NULL, TO_ROOM);
	return TRUE;
    }

    if (saves_spell(level,ch, vch,DAM_OTHER))
    {
        send_to_char("The earth fails to take hold of them.\n\r", ch);
        return TRUE;
    }

    if (is_flying(vch))
    {
        stop_flying(*vch);

        af.where     = TO_AFFECTS;
        af.type      = sn;
        af.level     = level;
        af.duration  = level/2;
        af.location  = 0;
        af.modifier  = 0;
        af.bitvector = 0;
        affect_to_char( vch, &af );

        send_to_char( "You fall to the ground with a thud!\n\r", vch );
        damage_old(ch,vch,dice(level,2),sn,DAM_BASH,TRUE);
    }
    else
        send_to_char("They are not flying.\n\r", ch);

    return TRUE;
}

bool spell_earthelementalsummon(int sn, int level, CHAR_DATA *ch, void *vo,int target)
{
    if (is_affected(ch, gsn_earthelementalsummon))
    {
        send_to_char("You don't feel you can summon another earth elemental yet.\n\r", ch);
        return FALSE;
    }

    // Check for any pets other than clockwork golems; this requires a traversal of the full char list
    for (CHAR_DATA * pet(char_list); pet != NULL; pet = pet->next)
    {
        if (IS_VALID(pet) && IS_NPC(pet) && pet->master == ch && pet->leader == ch
        && pet->pIndexData->vnum != MOB_VNUM_CLOCKWORKGOLEM)
        {
            send_to_char("You already have a loyal follower, and cannot bind more loyalties.\n", ch);
            return FALSE;
        }
    }

    if (ch->in_room == NULL || !ON_GROUND(ch))
    {
        send_to_char("There isn't enough earth here to summon an earth elemental.\n\r", ch);
        return FALSE;
    }

    CHAR_DATA * elemental = create_mobile(get_mob_index(MOB_VNUM_EARTH_ELEMENTAL));
    elemental->level = level;
    elemental->damroll = level / 2;
    elemental->hitroll = (level * 2) / 3;
    elemental->damage[0] = level / 2;
    elemental->damage[1] = 4;
    elemental->damage[2] = (level * 2) / 15;
    elemental->max_hit  = dice(level * 4, 15) + 200;
    elemental->hit = elemental->max_hit;

    char_to_room(elemental, ch->in_room);
    ch->pet = elemental;
    elemental->master = ch;
    elemental->leader = ch;

    for (DESCRIPTOR_DATA * d = descriptor_list; d != NULL; d = d->next)
    {
        if (d->connected == CON_PLAYING && d->character != NULL && d->character->in_room != NULL
        && d->character->in_room->area == ch->in_room->area)
            send_to_char("You feel the ground quake beneath you.\n", d->character);
    }

    act("With an unmistakable rumble, the ground cracks and forms into a burly earth elemental, which kneels before $n!", ch, NULL, NULL, TO_ROOM);
    act("With an unmistakable rumble, the ground cracks and forms into a burly earth elemental, which kneels before you!", ch, NULL, NULL, TO_CHAR);

    // Cooldown
    AFFECT_DATA af = {0};
    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = 65 / (ch->in_room->sector_type == SECT_UNDERGROUND ? 2 : 1);
    affect_to_char(ch, &af);
    return true;
}

bool spell_earthmaw( int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;

    if (!ch->in_room)
        return FALSE;

    if (victim == ch)
    {
	send_to_char("You cannot trap yourself within an earthy maw.\n\r", ch);
	return FALSE;
    }

    if (is_affected(ch, gsn_earthmaw))
    {
	send_to_char("You don't feel ready to open another earthen maw yet.\n\r", ch);
	return FALSE;
    }

    if (is_flying(victim))
    {
	send_to_char("Their feet are not on the ground.\n\r", ch);
	return FALSE;
    }

    if ((ch->in_room->sector_type == SECT_WATER_SWIM)
     || (ch->in_room->sector_type == SECT_WATER_NOSWIM)
     || (ch->in_room->sector_type == SECT_AIR)
     || (ch->in_room->sector_type == SECT_UNDERWATER))
    {
	send_to_char("You cannot open an earthen maw here.\n\r", ch);
	return FALSE;
    }

    if (((IS_NPC(victim) && IS_SET(victim->act, ACT_NOSUBDUE)) || (number_percent() < (get_curr_stat(victim, STAT_DEX) * 2.5))) && (victim != ch))
    {
        act("The earth opens up beneath $n, but $e leaps clear in time!", victim, NULL, NULL, TO_ROOM);
        send_to_char("The earth begins to open beneath you, but you leap clear!\n\r", victim);
        return TRUE;
    }

    if (create_earth_maw(*victim, level))
        WAIT_STATE(victim, UMAX(victim->wait, 3*PULSE_VIOLENCE));

    // Apply cooldown 
    AFFECT_DATA af = {0};
    af.where	= TO_AFFECTS;
    af.type     = sn;
    af.level    = level;
    af.duration	= 8;
    affect_to_char(ch, &af);

    return TRUE;
}


bool spell_earthquake( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
    CHAR_DATA *vch;
    CHAR_DATA *vch_next;

    send_to_char( "The earth trembles beneath your feet!\n\r", ch );
    act( "$n makes the earth tremble and shiver.", ch, NULL, NULL, TO_ROOM );

    for ( vch = char_list; vch != NULL; vch = vch_next )
    {
        vch_next        = vch->next;
        if ( vch->in_room == NULL )
            continue;
        if ( vch->in_room == ch->in_room )
        {
            if ( vch != ch && !is_safe_spell(ch,vch,TRUE) &&
                !is_same_group (ch, vch))
            {
                if (is_flying(vch) && !is_affected(vch, gsn_earthbind))
                    damage_old(ch,vch,0,sn,DAM_BASH,TRUE);
                else
                    damage_old( ch,vch,level + dice(2, 8), sn, DAM_BASH,TRUE);
            }
            continue;
        }

        if ( vch->in_room->area == ch->in_room->area )
            send_to_char( "The earth trembles and shivers.\n\r", vch );
    }

    return TRUE;
}


bool spell_fleshtostone( int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
    CHAR_DATA *victim = (CHAR_DATA *)vo;

    if (IS_NAFFECTED(victim, AFF_FLESHTOSTONE))
    {
        send_to_char("They're already encased in stone.\n\r", ch);
        return FALSE;
    }

    if (saves_spell(level, ch, victim, DAM_OTHER))
    {
        act("They resist your spell.", ch, NULL, NULL, TO_CHAR);
        act("You feel stone forming around your body, but you keep moving.", victim, NULL, NULL, TO_CHAR);
        return TRUE;
    }

    act("$N struggles as stone encases $S body!", ch, NULL, victim, TO_NOTVICT);
    act("$N struggles as stone encases $S body!", ch, NULL, victim, TO_CHAR);
    act("You are unable to even scream as stone forms around you, encasing your body!", ch, NULL, victim, TO_VICT);

    if (check_spirit_of_freedom(victim))
    {
        send_to_char("The spirit of freedom surges within you, and the encasing stone falls harmlessly away.\n\r", victim);
	act("The stone encasing $n quickly begins to crack, brilliant light glowing from within.", victim, NULL, NULL, TO_ROOM);
	act("The stone encasing $n falls away.", victim, NULL, NULL, TO_ROOM);
	return TRUE;
    }

    stop_fighting_all(victim);

    af.where     = TO_NAFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = level/16;
    af.location  = 0;
    af.modifier  = 0;
    af.bitvector = AFF_FLESHTOSTONE;
    affect_to_char(victim, &af);

    af.where	 = TO_PAFFECTS;
    af.bitvector = AFF_AIRLESS;
    affect_to_char(victim, &af);

    SET_BIT(victim->act, PLR_FREEZE);
    return TRUE;
}

bool spell_fortify( int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    victim = (CHAR_DATA *) vo;


    if ( is_affected( victim, sn ) )
    {
        if (victim == ch)
          send_to_char("You already feel earthy.\n\r",ch);
        else
          act("$N has already taken on the endurance of the earth.",
        ch,NULL,victim,TO_CHAR);
        return FALSE;
    }

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = (6 + level) * (check_durablemagics(*ch) ? 2 : 1);
    af.location  = APPLY_HIT;
    af.modifier  = level * 2;
    af.bitvector = 0;
    affect_to_char( victim, &af );
    victim->hit += level*2;

    af.location  = APPLY_SAVING_SPELL;
    af.modifier  = 0 - level / 8;
    affect_to_char( victim, &af );
    send_to_char( "You feel earthy.\n\r", victim );
    if ( ch != victim )
        act("You grant fortification to $N.",ch,NULL,victim,TO_CHAR);
    return TRUE;
}

bool spell_giant_strength(int sn,int level,CHAR_DATA *ch,void *vo,int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if ( is_affected( victim, sn ) )
    {
        if (victim == ch)
          send_to_char("You are already as strong as you can get!\n\r",ch);
        else
          act("$N can't get any stronger.",ch,NULL,victim,TO_CHAR);
        return FALSE;
    }

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = level;
    af.location  = APPLY_STR;
    af.modifier  = 1 + (level >= 18) + (level >= 25) + (level >= 32);
    af.bitvector = 0;
    affect_to_char( victim, &af );
    send_to_char( "Your muscles surge with heightened power!\n\r", victim );
    act("$n's muscles surge with heightened power.",victim,NULL,NULL,TO_ROOM);
    return TRUE;
}


bool spell_gravitywell( int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    if (is_affected(ch, gsn_gravitywell))
    {
        send_to_char("You cannot summon the power to call a gravity well again yet.\n\r", ch);
        return FALSE;
    }

    if (area_is_affected(ch->in_room->area, gsn_gravitywell))
    {
        send_to_char("This place is already pulled down with the force of a gravity well.\n\r", ch);
        return FALSE;
    }

    AFFECT_DATA af = {0};
    af.where     = TO_AREA;
    af.type	 = sn;
    af.level     = level;
    af.duration  = 6;
    affect_to_area(ch->in_room->area, &af);

    int skill(get_skill(ch, sn));
    af.duration     = 120 - UMAX(0, skill - 70);
    affect_to_char(ch, &af);

    for (DESCRIPTOR_DATA * d = descriptor_list; d != NULL; d = d->next)
    {
        if (d->connected == CON_PLAYING && d->character && !IS_IMMORTAL(d->character) 
        && d->character->in_room && d->character->in_room->area == ch->in_room->area)
        {
            send_to_char("You feel pulled down as everything becomes heavy!\n\r", d->character);
            if (is_flying(d->character) && d->character != ch && !check_spirit_of_freedom(d->character))
            {
                stop_flying(*d->character);
                send_to_char("You go crashing to the ground!\n\r", d->character);
            }
        }
    }

    return TRUE;
}

bool spell_knock( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
int direction = 10;
OBJ_DATA *obj = NULL;
EXIT_DATA *pexit;

    if (!ch->in_room)
	return FALSE;

    if (target_name[0] == '\0')
    {
        send_to_char("Knock what?\n\r", ch);
        return FALSE;
    }

    if ( !str_prefix( target_name, "north" ) ) direction = 0;
    else if (!str_prefix (target_name, "east") ) direction = 1;
    else if (!str_prefix (target_name, "south") ) direction = 2;
    else if (!str_prefix (target_name, "west") ) direction = 3;
    else if (!str_prefix (target_name, "up") ) direction = 4;
    else if (!str_prefix (target_name, "down") ) direction = 5;
    else
    {
	if ((obj = get_obj_list(ch, target_name, ch->in_room->contents)) == NULL)
        {
            send_to_char("You can't see that to manipulate its lock.\n\r", ch);
            return FALSE;
        }
    }

    if (direction < 10)
    {
        if ((pexit = ch->in_room->exit[direction]) == NULL)
        {
            send_to_char("There's no door there!\n\r", ch);
            return FALSE;
        }
        else if (!pexit->exit_info & EX_CLOSED)
        {
            send_to_char("You can only knock a closed door.\n\r", ch);
            return FALSE;
        }
        else if (pexit->exit_info & EX_PICKPROOF)
        {
            send_to_char("You failed.\n\r", ch);
            return TRUE;
        }
	else if (pexit->key == 0)
	{
	    send_to_char("That door has no lock you can manipulate.\n\r",ch);
	    return FALSE;
	}
        else if (pexit->exit_info & EX_RUNEOFEARTH)
        {
            act("$n's magic is absorbed unequivocably by the rune of earth, and the door remains still.", ch, NULL, NULL, TO_ROOM);
            act("Your magic is absorbed unequivocably by the rune of earth, and the door remains still.", ch, NULL, NULL, TO_CHAR);
            return TRUE;
        }
        if (number_percent() > (75 - level))
        {
            if (pexit->exit_info & EX_LOCKED)
	    {
		send_to_char("You sense the tumblers of the lock open.\n\r", ch);
        	REMOVE_BIT(pexit->exit_info, EX_LOCKED);
	    }
	    else
	    {
		send_to_char("You sense the tumblers of the lock close.\n\r",ch);
		SET_BIT(pexit->exit_info,EX_LOCKED);
	    }
            return TRUE;
        }
        else
        {
            send_to_char("You failed.\n\r", ch);
            return TRUE;
        }
    }

    if (obj != NULL && obj->item_type != ITEM_CONTAINER)
    {
        send_to_char("That's not a container!\n\r", ch);
        return FALSE;
    }

    if (!obj->value[1] & CONT_CLOSED)
    {
        send_to_char("That's not closed.\n\r", ch);
        return FALSE;
    }

    if (obj->value[1] & CONT_PICKPROOF)
    {
        send_to_char("You failed.\n\r", ch);
        return TRUE;
    }
    
    if (obj->value[2] == 0)
    {
	send_to_char("That container has no lock you can manipulate.\n\r",ch);
	return FALSE;
    }

    if (number_percent() > (75 - level))
    {
	if (IS_SET(obj->value[1], CONT_LOCKED))
	{
	    send_to_char("You sense the tumblers of the lock open.\n\r", ch);
            REMOVE_BIT(obj->value[1], CONT_LOCKED);
	}
	else
	{
	    send_to_char("You sense the tumblers of the lock close.\n\r",ch);
	    SET_BIT(obj->value[1], CONT_LOCKED);
	}
        return TRUE;
    }
    else
    {
        send_to_char("You failed.\n\r", ch);
        return TRUE;
    }
}

bool spell_magneticgrasp( int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    OBJ_DATA *weapon,*offhand;
    int elevel = level - 1;
    int bw=0;

    weapon = get_eq_char(victim, WEAR_WIELD);

    if (weapon 
      && (IS_SET(weapon->extra_flags[0], ITEM_NOREMOVE)
     || IS_SET(weapon->extra_flags[0], ITEM_NODISARM)
     || !material_table[weapon->material].metal))
    	weapon = NULL;

    offhand = get_eq_char(victim, WEAR_DUAL_WIELD);
    if (offhand 
      && (IS_SET(offhand->extra_flags[0], ITEM_NOREMOVE)
      || IS_SET(offhand->extra_flags[0], ITEM_NODISARM)
      || !material_table[offhand->material].metal))
	offhand = NULL;
    
    if (offhand == NULL)
    {
	offhand = get_eq_char(victim, WEAR_SHIELD);
        if (offhand
          && (IS_SET(offhand->extra_flags[0], ITEM_NOREMOVE)
          || IS_SET(offhand->extra_flags[0], ITEM_NODISARM)
          || !material_table[offhand->material].metal))
	    offhand = NULL;
    }

    if (offhand == NULL)
    {
	offhand = get_eq_char(victim, WEAR_HOLD);
        if (offhand
          && (IS_SET(offhand->extra_flags[0], ITEM_NOREMOVE)
          || IS_SET(offhand->extra_flags[0], ITEM_NODISARM)
          || !material_table[offhand->material].metal))
	    offhand = NULL;
    }

    if (weapon == NULL && offhand == NULL)
    {
	if (ch == victim)
	    act("You have nothing you can repel.",ch,NULL,victim,TO_CHAR);
	else
	    act("$N has nothing you can repel.",ch,NULL,victim,TO_CHAR);
	return TRUE;
    }
    else
    {
	act("You reach out towards $N, focusing upon $S weapons.", ch, NULL, victim, TO_CHAR);
	act("$n reaches out, $s palm extended towards you.", ch, NULL, victim, TO_VICT);
	act("$n reaches out, $s palm extended towards $N.", ch, NULL, victim, TO_NOTVICT);
    }

    if (is_affected(victim, gsn_clumsiness))
	elevel += 5;

    if (is_affected(victim, gsn_grip))
	elevel -= 10;
    
    if (offhand)
    {
	if (obj_is_affected(offhand, gsn_bindweapon))
	    bw = -10;
        if (saves_spell(elevel + bw, ch, victim, DAM_OTHER))
	{
	    act("$p trembles in your grasp, but you maintain a firm grip on it.", ch, offhand, victim, TO_VICT);
	    act("$p trembles in $n's grasp, but $e maintains a firm grip on it.", victim, offhand, NULL, TO_ROOM);
    	}
	else
	{
	    act("$p flies from your grasp!", ch, offhand, victim, TO_VICT);
	    act("$p flies from $N's grasp!", ch, offhand, victim, TO_CHAR);
	    act("$p flies from $N's grasp!", ch, offhand, victim, TO_NOTVICT);

	    obj_from_char(offhand);
	    if (IS_SET(offhand->extra_flags[0],ITEM_NODROP)
	      || IS_SET(offhand->extra_flags[0],ITEM_INVENTORY))
		obj_to_char(offhand, victim);
	    else
		obj_to_room(offhand, ch->in_room);
	}
    }
    bw = 0;
    if (weapon)
    {
	if (obj_is_affected(weapon, gsn_bindweapon))
	    bw = -10;
        if (saves_spell(elevel + bw, ch, victim, DAM_OTHER))
	{
	    act("$p trembles in your grasp, but you maintain a firm grip on it.", ch, weapon, victim, TO_VICT);
	    act("$p trembles in $n's grasp, but $e maintains a firm grip on it.", victim, weapon, NULL, TO_ROOM);
    	}
	else
	{
	    act("$p flies from your grasp!", ch, weapon, victim, TO_VICT);
	    act("$p flies from $N's grasp!", ch, weapon, victim, TO_CHAR);
	    act("$p flies from $N's grasp!", ch, weapon, victim, TO_NOTVICT);

	    obj_from_char(weapon);
	    if (IS_SET(weapon->extra_flags[0],ITEM_NODROP)
	      || IS_SET(weapon->extra_flags[0],ITEM_INVENTORY))
		obj_to_char(weapon, victim);
	    else
		obj_to_room(weapon, ch->in_room);
	}
    }

    return TRUE;
}

bool spell_meldwithstone(int sn, int level, CHAR_DATA *ch, void *vo,int target)
{
    if (ch->in_room == NULL || !ON_GROUND(ch))
    {
        send_to_char("You aren't in touch with the ground enough here to meld with the earth.\n", ch);
        return false;
    }

    act("With a word of power, $n melts away and the ground opens to swallow $m.", ch, NULL, NULL, TO_ROOM);
    act("With a word of power, you melt away, and the ground opens to swallow you.", ch, NULL, NULL, TO_CHAR);

    AFFECT_DATA af = {0};
    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = -1;
    affect_to_char(ch, &af);
 
    if (ch->pet != NULL && ch->pet->in_room == ch->in_room && Drakes::SpecialCount(*ch->pet, Drakes::Ambush) > 0)
    {
        act("$N flows down into the earth alongside you.", ch, NULL, ch->pet, TO_CHAR);
        act("$N flows down into the earth.", ch, NULL, ch->pet, TO_NOTVICT);
        affect_to_char(ch->pet, &af);
    }

    return true;
}

bool spell_metaltostone( int sn, int level,CHAR_DATA *ch,void *vo,int target)
{
    CHAR_DATA * victim(static_cast<CHAR_DATA*>(vo));
    OBJ_DATA * obj(get_eq_char(victim, WEAR_WIELD));
    if (obj == NULL)
    {
        obj = get_eq_char(victim, WEAR_DUAL_WIELD);
        if (obj == NULL)
        {
            if (ch == victim) send_to_char("You aren't wielding a weapon.\n", ch);
            else act("$N isn't wielding a weapon.", ch, NULL, victim, TO_CHAR);
            return false;
        }
    }

    if (obj_is_affected(obj, gsn_metaltostone))
    {
        act("$p has already been turned to stone.", ch, obj, NULL, TO_CHAR);
        return false;
    }

    if (!material_table[obj->material].metal)
    {
        act("$p is not metal!", ch, obj, NULL, TO_CHAR);
        return false;
    }

    if (IS_OBJ_STAT(obj, ITEM_NODESTROY))
    {
        act("$p begins to turn to stone but quickly returns to normal.", ch, obj, NULL, TO_CHAR);
        return false;
    }

    if (saves_spell(level, ch, victim, DAM_OTHER))
    {
        act("$p begins to turn to stone, but returns to normal.", victim, obj, NULL, TO_CHAR);
        act("$p begins to turn to stone, but returns to normal.", victim, obj, NULL, TO_ROOM);
        return true;
    }

    AFFECT_DATA af = {0};
    af.where        = TO_OBJECT;
    af.type         = sn;
    af.level        = level;
    af.duration     = (level * (check_durablemagics(*ch) ? 2 : 1)) / 2;
    af.location     = APPLY_WEIGHT;
    af.modifier     = obj->weight*2;
    affect_to_obj(obj,&af);

    act("$p trembles and crackles as it turns to stone.", ch, obj, NULL, TO_CHAR);
    act("$p trembles and crackles as it turns to stone.", ch, obj, NULL, TO_ROOM);
    return true;
}

bool spell_meteorstrike( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    if (victim->in_room == NULL || !IS_OUTSIDE(victim))
    {
        send_to_char("You can't call a meteor from inside!\n", ch);
        return false;
    }

    if (victim->in_room->sector_type == SECT_UNDERWATER)
    {
        send_to_char("You cannot call a meteor from beneath the water!\n", ch);
        return false;
    }
	
    act("$n calls a meteor from the skies to strike $N!", ch, NULL, victim, TO_NOTVICT);
    act("You call a meteor from the skies to strike $N!", ch, NULL, victim, TO_CHAR);
    act("$n calls a meteor from the skies to strike you!", ch, NULL, victim, TO_VICT);

    int baseDam(30 + level);
    int dam(number_range(baseDam * 3,  baseDam * 5) / 2);
    if (saves_spell( level, ch, victim, DAM_BASH))
        dam /= 2;

    damage_old(ch, victim, dam, sn, DAM_BASH, TRUE);
    return true;
}

bool spell_mysticanchor( int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if (is_affected(ch, gsn_anchor))
    {
	send_to_char("You are already mystically anchored in place.\n\r", ch);
	return FALSE;
    }
    if (ch->in_room->sector_type == SECT_AIR 
      || ch->in_room->sector_type == SECT_WATER_NOSWIM 
      || ch->in_room->sector_type == SECT_WATER_SWIM 
      || ch->in_room->sector_type == SECT_UNDERWATER)
    {
        send_to_char("You aren't in touch with the ground enough here to anchor yourself.\n\r", ch);
        return FALSE;
    }

    af.where	 = TO_AFFECTS;
    af.type	 = sn;
    af.level	 = level;
    af.duration  = level/6;
    af.location  = APPLY_NONE;
    af.modifier  = 0;
    af.bitvector = 0;
    affect_to_char(ch, &af);

    send_to_char("You call upon the earth to anchor yourself against opposing forces.\n\r", ch);
    act("$n appears to be more steady.", ch, NULL, NULL, TO_ROOM);

    return TRUE;
}

bool spell_petrify( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if (IS_OAFFECTED(victim, AFF_PETRIFY))
    {
	act("$N is already affected by petrification.", ch, NULL, victim, TO_CHAR);
	return FALSE;
    }

    if (saves_spell(level, ch, victim, DAM_OTHER))
    {
	send_to_char("Your muscles freeze momentarily, but you shake it off.\n\r", victim);
	act("$N is unaffected by your petrification!", ch, NULL, victim, TO_CHAR);
	return TRUE;
    }

    af.where	 = TO_OAFFECTS;
    af.type	 = sn;
    af.level	 = level;
    af.duration	 = level/5;
    af.location  = APPLY_NONE;
    af.modifier  = 0;
    af.bitvector = AFF_PETRIFY;
    affect_to_char(victim, &af);

    send_to_char("Your muscles begin to harden and freeze, making movement difficult!\n\r", victim);
    act("$n begins to move unsteadily as petrification overtakes $m!", victim, NULL, NULL, TO_ROOM);

    if (!IS_AFFECTED(victim, AFF_SLOW))
    {
	af.where     = TO_AFFECTS;
	af.location  = APPLY_DEX;
        af.modifier  = -1 - (level >= 18) - (level >= 25) - (level >= 32);
	af.bitvector = AFF_SLOW;
	affect_to_char(victim, &af);
	send_to_char("You feel yourself slow down.\n\r", victim);
	act("$n appears to be moving more slowly.", victim, NULL, NULL, TO_ROOM);
    }

    return TRUE;
}

bool spell_protnormalmissiles(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if (is_affected(ch, gsn_protnormalmissiles))
    {
	send_to_char("You are already protected from ranged attacks.\n\r", ch);
	return FALSE;
    }

    af.where	 = TO_AFFECTS;
    af.type	 = sn;
    af.level	 = level;
    af.duration	 = (level / 3) + 3;
    af.location	 = APPLY_NONE;
    af.modifier	 = 0;
    af.bitvector = 0;
    affect_to_char(ch, &af);

    send_to_char("You feel protected.\n\r", ch);

    return TRUE;
}


bool spell_quicksand( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    CHAR_DATA *vch;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
    char buf[MAX_STRING_LENGTH];   
 
    if (!ch->in_room || !victim->in_room)
	return FALSE;

    for (vch = victim->in_room->people; vch; vch = vch->next_in_room)
    {
	if (((ch != vch) || ((ch == vch) && (ch == victim)))
	 && ((vch->master == victim) || is_same_group(vch, victim))
	 && !is_safe_spell(ch, vch, TRUE)
	 && !is_flying(vch)
	 && !(IS_AFFECTED(vch, AFF_WIZI) || (!IS_IMMORTAL(ch) && IS_IMMORTAL(vch)))
	 && (IS_NPC(vch) || IS_PK(vch, ch)))
	{
	    send_to_char("Quicksand begins to form beneath your feet, hindering your movement!\n\r", vch);
	    act("Quicksand forms beneath $n, and $e begins to sink!", vch, NULL, NULL, TO_ROOM);

            if ((vch != victim)
	     && !IS_NPC(vch)
	     && (!ch->fighting || !vch->fighting))
            {
                if (can_see(vch, ch))
                    sprintf(buf, "Help!  %s is attacking me!", PERS(ch, vch));
                else
                    sprintf(buf, "Help!  Someone is attacking me!");
                do_autoyell(vch, buf);
            }
            check_killer(ch, vch);
	   
	    af.where	 = TO_AFFECTS;
	    af.type	 = sn;
	    af.level	 = level;
	    af.duration  = level / 5;
	    af.location  = APPLY_DEX;
	    af.modifier  = -1;
	    af.bitvector = 0;
	    affect_to_char(vch, &af);
	}
    }

    return TRUE;
}
	
bool spell_runeofearth( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
    int direction;
    EXIT_DATA *pexit;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
    char buf[MAX_STRING_LENGTH];

    if (!ch->in_room)
	return FALSE;

    if (target_name[0] == '\0')
    {
        send_to_char("Place a rune of earth where?\n\r", ch);
        return FALSE;
    }

	if (!str_prefix(target_name, "north"))	direction = 0;
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

    if ((pexit = ch->in_room->exit[direction]) == NULL)
    {
	send_to_char("There's no exit there!\n\r", ch);
        return FALSE;
    }

    if (!IS_SET(pexit->exit_info, EX_ISDOOR))
    {
        send_to_char("There's no door in that direction!\n\r", ch);
        return FALSE;
    }

    if (!IS_SET(pexit->exit_info, EX_CLOSED))
    {
	send_to_char("That door is not closed!\n\r", ch);
	return FALSE;
    }

    if (IS_SET(pexit->exit_info, EX_RUNEOFEARTH))
    {
	send_to_char("A rune of earth already blocks that exit.\n\r", ch);
	return FALSE;
    }

    SET_BIT(ch->in_room->exit[direction]->exit_info, EX_RUNEOFEARTH);

    if (ch->in_room->exit[direction]->u1.to_room->exit[OPPOSITE(direction)]
     && ch->in_room->exit[direction]->u1.to_room->exit[OPPOSITE(direction)]->u1.to_room == ch->in_room
     && pexit->u1.to_room->exit[OPPOSITE(direction)]->exit_info & EX_ISDOOR)
    {
        SET_BIT(pexit->u1.to_room->exit[OPPOSITE(direction)]->exit_info, EX_RUNEOFEARTH);

        if (ch->in_room->exit[direction]->u1.to_room->people)
        {
            sprintf(buf, "A rune of earth appears on the door %s.",
            ((OPPOSITE(direction) == 0) ? "to the north" : (OPPOSITE(direction) == 1) ? "to the east" :
             (OPPOSITE(direction) == 2) ? "to the south" : (OPPOSITE(direction) == 3) ? "to the west" :
             (OPPOSITE(direction) == 4) ? "above you" : "below you"));

            act(buf, ch->in_room->exit[direction]->u1.to_room->people, NULL, NULL, TO_ROOM);
            act(buf, ch->in_room->exit[direction]->u1.to_room->people, NULL, NULL, TO_CHAR);
        }
    }

    sprintf(buf, "A rune of earth appears on the door %s.",
	((direction == 0) ? "to the north" : (direction == 1) ? "to the east" :
	 (direction == 2) ? "to the south" : (direction == 3) ? "to the west" :
	 (direction == 4) ? "below you" : "above you"));

    act(buf, ch, NULL, NULL, TO_ROOM);
    act(buf, ch, NULL, NULL, TO_CHAR);

    af.where	 = TO_ROOM_AFF;
    af.type	 = gsn_runeofearth;
    af.level     = level;
    af.duration  = (level * (check_durablemagics(*ch) ? 2 : 1)) / 2;
    af.location	 = 0;
    af.modifier	 = direction;
    af.bitvector = 0;
    affect_to_room(ch->in_room, &af);

    return TRUE;
}

bool spell_sandspray( int sn, int level, CHAR_DATA *ch,void *vo,int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;

    int duration(1);
    if (number_percent() <= determine_saltoftheearth_level(*ch, SPH_AIR))
    {
        level += 4;
        duration += 2;
    }

    int dam = dice(level, 3);
    if (saves_spell(level, ch, victim, DAM_SLASH))
        dam /= 2;

    act("$n calls up a blast of hot sand to sear $N.", ch, NULL, victim, TO_NOTVICT);
    act("You call up a blast of hot sand to sear $N.", ch, NULL, victim, TO_CHAR);
    act("$n calls up a blast of hot sand to sear you.", ch, NULL, victim, TO_VICT);
    damage_old(ch, victim, dam, sn, DAM_SLASH, true);

    if (!IS_VALID(victim) || IS_OAFFECTED(victim, AFF_GHOST))
        return TRUE;

    if (IS_SET(victim->imm_flags, IMM_BLIND) || IS_AFFECTED(victim, AFF_BLIND))
        return TRUE;

    if (saves_spell(level, ch, victim, DAM_OTHER) || number_percent() <= 35)
        return TRUE;

    act ("$n is blinded by the sand in $s eyes!", victim, NULL, NULL, TO_ROOM);
    act ("You are blinded by the sand in your eyes!", victim, NULL, NULL, TO_CHAR);

    AFFECT_DATA af = {0};
    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.location  = APPLY_HITROLL;
    af.modifier  = -4;
    af.duration  = duration + (check_durablemagics(*ch) ? 2 : 0);
    af.bitvector = AFF_BLIND;
    affect_to_char( victim, &af );

    return TRUE;
}

static bool shape_helper(int sn, int level, int effectLocation, int bonusMultiplier, CHAR_DATA & ch, OBJ_DATA & obj)
{
    // Verify not worn
    if (obj.worn_on)
    {
        act("You cannot focus on shaping $p while using it!", &ch, &obj, NULL, TO_CHAR);
        return false;
    }

    // Verify not nodestroy
    if (IS_SET(obj.extra_flags[0], ITEM_NODESTROY))
    {
        act("$p is too tough to respond to your magics.", &ch, &obj, NULL, TO_CHAR);
        return false;
    }

    // Determine existing shape count
    int existingBonus(0);
    for (const AFFECT_DATA * paf(get_obj_affect(&obj, sn)); paf != NULL; paf = get_obj_affect(&obj, sn, paf))
    {
        if (paf->location == effectLocation)
            existingBonus += paf->modifier / bonusMultiplier;
    }

    // Calculate odds of failure
    int skill(get_skill(&ch, sn));
    int failOdds(25);
    failOdds += existingBonus * existingBonus * 9;
    failOdds -= UMAX(0, skill - 75);
    if (IS_OBJ_STAT(&obj, ITEM_BLESS)) failOdds -= 5;

    if (number_percent() <= get_skill(&ch, gsn_forgemaster))
    {
        check_improve(&ch, NULL, gsn_forgemaster, true, 8);
        failOdds -= 25;
    }
    else
        check_improve(&ch, NULL, gsn_forgemaster, false, 8);

    switch (Luck::Check(ch))
    {
        case Luck::Lucky: failOdds -= 50; break;
        case Luck::Unlucky: failOdds += 50; break;
        default: break;
    }

    failOdds = URANGE(5, failOdds, 85);

    // Calculate success
    int result(number_percent());
    if (result < failOdds)
    {
        if ((failOdds - result) > 25)
        {
            // Critical failure, item destroyed
            if (number_bits(1) == 0)
            {
                act("$p shakes violently, then shatters!", &ch, &obj, NULL, TO_CHAR);
                act("$p shakes violently, then shatters!", &ch, &obj, NULL, TO_ROOM);
            }
            else
            {
                act("$p quivers, turning quickly to mud and melting away!", &ch, &obj, NULL, TO_CHAR);
                act("$p quivers, turning quickly to mud and melting away!", &ch, &obj, NULL, TO_ROOM);
            }
            extract_obj(&obj);
            return true;
        }
        
        // Simple failure, no result
        act("$p trembles momentarily, but fails to reshape.", &ch, &obj, NULL, TO_CHAR);
        act("$p trembles momentarily, but fails to reshape.", &ch, &obj, NULL, TO_ROOM);
        return true;
    }

    // Success
    if (number_bits(1) == 0)
    {
        act("$p cracks and squirms, then solidifies.", &ch, &obj, NULL, TO_CHAR);
        act("$p cracks and squirms, then solidifies.", &ch, &obj, NULL, TO_ROOM);
    }
    else
    {
        act("$p turns momentarily soft, rehardening with improved form.", &ch, &obj, NULL, TO_CHAR);
        act("$p turns momentarily soft, rehardening with improved form.", &ch, &obj, NULL, TO_ROOM);
    }

    // Set the magic bit and increase the level, if below hero
    SET_BIT(obj.extra_flags[0], ITEM_MAGIC);
    if (obj.level < LEVEL_HERO)
        obj.level = UMIN(LEVEL_HERO - 1, obj.level + 1);

    // Increase the bonus
    AFFECT_DATA af = {0};
    af.where    = TO_OBJECT;
    af.type     = sn;
    af.level    = level;
    af.duration = -1;
    af.location = effectLocation;
    af.modifier = bonusMultiplier;
    obj_affect_join(&obj, &af);

    // Increase the weight
    obj.weight += 30;
    ch.carry_weight += 30;
    return true;
}

bool spell_shapematter(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    // Dispatch according to item type
    OBJ_DATA * obj(static_cast<OBJ_DATA*>(vo));
    switch (obj->item_type)
    {
        case ITEM_ARMOR: return shape_helper(gsn_shapearmor, level, APPLY_HIT, 3, *ch, *obj);
        case ITEM_WEAPON: return shape_helper(gsn_shapeweapon, level, APPLY_DAMROLL, 1, *ch, *obj);
    }

    // Unknown type
    send_to_char("You only know how to shape weapons and armor.\n", ch);
    return false;
}

bool spell_shapearmor( int sn, int level, CHAR_DATA *ch, void *vo,int target)
{
    // Verify item type
    OBJ_DATA *obj = (OBJ_DATA *) vo;
    if (obj->item_type != ITEM_ARMOR)
    {
        act("$p is not armor.", ch, obj, NULL, TO_CHAR);
        return false;
    }

    return shape_helper(sn, level, APPLY_HIT, 3, *ch, *obj);
}

bool spell_shapeweapon( int sn, int level, CHAR_DATA *ch, void *vo,int target)
{
    // Verify item type
    OBJ_DATA *obj = (OBJ_DATA *) vo;
    if (obj->item_type != ITEM_WEAPON)
    {
        act("$p is not a weapon.", ch, obj, NULL, TO_CHAR);
        return false;
    }

    return shape_helper(sn, level, APPLY_DAMROLL, 1, *ch, *obj);
}

bool spell_shatter( int sn, int level,CHAR_DATA *ch,void *vo,int target)
{
    OBJ_DATA *obj = NULL, *pobj;
    int x = 0;
    CHAR_DATA *victim = (CHAR_DATA *)vo;

    if (is_safe_spell(victim, ch, FALSE))
    {
        act("The gods protect $n.", victim, NULL, NULL, TO_ROOM);
        act("The gods protect you.", victim, NULL, NULL, TO_CHAR);
        return FALSE;
    }

    while (obj == NULL && x < 20)
        for (pobj = victim->carrying, x++; pobj != NULL; pobj = pobj->next_content)
        {
//            if (pobj->wear_loc != -1 && number_bits(4) == 0)

	    if (pobj->worn_on && number_bits(4) == 0)
            {
                obj = pobj;
                break;
            }
        }

    if (obj == NULL)
    {
        send_to_char("You failed.\n\r", ch);
        return TRUE;
    }

    if (saves_spell( level, ch, victim, DAM_OTHER) || IS_SET(obj->extra_flags[0], ITEM_NODESTROY) || number_bits(1) == 0)
    {
        send_to_char("You try to shatter their gear, but fail.\n\r", ch);
        act("You feel $p tremble, but it subsides.", victim, obj, NULL, TO_CHAR);
        return TRUE;
    }

    act("$p trembles and explodes!", ch, obj, victim, TO_CHAR);
    act("$p trembles and explodes!", ch, obj, victim, TO_ROOM);
    extract_obj(obj);
    return TRUE;
}


bool spell_slip( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;

    if (ch->in_room->sector_type == SECT_AIR
     || ch->in_room->sector_type == SECT_WATER_NOSWIM
     || ch->in_room->sector_type == SECT_WATER_SWIM
     || ch->in_room->sector_type == SECT_UNDERWATER)
    {
        send_to_char("You can only cause those on dry land to slip!\n\r", ch);
        return FALSE;
    }

    if (is_flying(victim))
    {
	act("$N's feet are not on the ground.", ch, NULL, victim, TO_CHAR);
        return FALSE;
    }

    if (saves_spell(level+4, ch, victim, DAM_OTHER))
    {
        act("$n turns the ground to mud, but $N gets away without slipping.", ch, NULL, victim, TO_NOTVICT);
        act("You turn the ground to mud, but $N gets away without slipping.", ch, NULL, victim, TO_CHAR);
        act("$n turns the ground to mud, but you get away without slipping.", ch, NULL, victim, TO_VICT);
        return TRUE;
    }

    act("$n turns the ground to mud, and $N slips and goes down in a tumble!", ch, NULL, victim, TO_NOTVICT);
    act("You turn the ground to mud, and $N slips and goes down in a tumble!", ch, NULL, victim, TO_CHAR);
    act("$n turns the ground to mud, and you slip and go down in a tumble!", ch, NULL, victim, TO_VICT);
    WAIT_STATE(victim, UMAX(victim->wait, (5*PULSE_VIOLENCE)/2));

    return TRUE;
}

bool spell_smoothterrain(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    AFFECT_DATA af = {0};
    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = 24 * (check_durablemagics(*ch) ? 2 : 1);
 
    unsigned int count(0);
    for (CHAR_DATA * gch = ch->in_room->people; gch != NULL; gch = gch->next_in_room )
    {
        if (!is_same_group(gch, ch))
            continue;

        ++count;
        affect_strip(gch, sn);	
        send_to_char("You feel the earth settle a bit beneath your feet.\n", gch);
        affect_to_char(gch, &af);
   }
    
    if (count > 1)
        send_to_char( "You settle the terrain beneath the feet of your companions.\n", ch);

    return true;
}

bool spell_stabilize(int sn,int level,CHAR_DATA *ch,void *vo, int target)
{
    if ( is_affected(ch, gsn_stabilize))
    {
        send_to_char("You are already rooted to the earth.\n\r",ch);
        return FALSE;
    }

    if (ch->in_room == NULL || !ON_GROUND(ch))
    {
        send_to_char("You aren't in touch with the ground enough here to stabilize yourself.\n\r", ch);
        return FALSE;
    }

    act ("You feel yourself take root using the earth.", ch, NULL, NULL, TO_CHAR);
    act ("$n looks more stable in $s position.", ch, NULL, NULL, TO_ROOM);

    AFFECT_DATA af = {0};
    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = (level * (check_durablemagics(*ch) ? 2 : 1)) / 2;
    affect_to_char(ch, &af );
    return true;
}

bool spell_stonefist( int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    if (ch->in_room == NULL || !ON_GROUND(ch))
    {
        send_to_char("You are not in touch enough with the earth here to call forth a stone fist.\n\r", ch);
        return FALSE;
    }

    act ("$n calls a huge hand from the ground!", ch,NULL,victim,TO_ROOM);
    act ("$N is crushed by a granite fist!",NULL,NULL,victim,TO_ROOM);

    std::vector<DamageInfo> damage;
    damage.push_back(DamageInfo(dice(level, 7), DAM_BASH));

    int saltLevel(determine_saltoftheearth_level(*ch, SPH_FIRE));
    if (saltLevel > 0)
        damage.push_back(DamageInfo(dice(2, 1 + (saltLevel / 3)), DAM_FIRE));

    bool saved(false);
    if (saves_spell(level, ch, victim, DAM_BASH))
    {
        damage[0].amount /= 2;
        saved = true;
    }

    damage_new(ch, victim, damage, sn, true);
    if (!saved) 
    {
        act("Your stonefist pummels $N to the ground!", ch, NULL, victim, TO_CHAR);
        act("You are pummeled into the ground by $n's granite fist!", ch, NULL, victim, TO_VICT);
        act("$n's stonefist pounds $N into the ground!", ch, NULL, victim, TO_NOTVICT);
        WAIT_STATE(victim, (3 * PULSE_VIOLENCE / 2));
    }

    return true;
}

bool spell_stoneshell(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    if (is_affected(ch, gsn_stoneshell))
    {
        send_to_char("You are already encased in a stone shell.\n", ch);
        return false;
    }

    AFFECT_DATA af = {0};
    af.where     = TO_AFFECTS;
    af.type      = gsn_stoneshell;
    af.level     = level;
    af.duration  = level / 7;
    af.modifier  = -3 * level;
    af.location  = APPLY_AC;
    affect_to_char(ch, &af);

    act("A shell of stone forms around you.", ch, NULL, NULL, TO_CHAR);
    act("A shell of stone forms around $n.", ch, NULL, NULL, TO_ROOM);
    return true;
}

bool spell_mantleofearth( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    OBJ_DATA *mantle;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if (is_affected(ch, sn))
    {
        send_to_char("You don't feel ready to create another mantle of earth yet.\n\r", ch);
        return FALSE;
    }

    if (!(mantle = create_object(get_obj_index(OBJ_VNUM_MANTLE_EARTH), level)))
    {
        bug("Spell: mantle of earth.  Cannot load mantle object.", 0);
        send_to_char("Something seems to be amiss...\n\r", ch);
        return FALSE;
    }

    mantle->level       = level;
    mantle->value[0]    = level/5;
    mantle->value[1]    = level/5;
    mantle->value[2]    = level/5;
    mantle->value[3]    = level/5 - 2;

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = 25;
    af.location  = 0;
    af.modifier  = 0;
    af.bitvector = 0;
    affect_to_char(ch, &af);

    send_to_char("Calling upon the powers of the earth, you create a mantle of stone.\n\r", ch);
    act("$n concentrates, and a mantle of stone forms before your eyes.", ch, NULL, NULL, TO_ROOM);

    af.where	= TO_OBJECT;
    af.duration = -1;
    af.location = APPLY_HITROLL;
    af.modifier = level/10;
    affect_to_obj(mantle,&af);
   
    af.location = APPLY_DAMROLL;
    affect_to_obj(mantle,&af);

    obj_to_char(mantle, ch);

    return TRUE;
}
    

bool spell_stone_skin( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if ( is_affected( ch, sn ) )
    {
        if (victim == ch)
          send_to_char("Your skin is already as hard as a rock.\n\r",ch);
        else
          act("$N is already as hard as can be.",ch,NULL,victim,TO_CHAR);
        return FALSE;
    }

    af.where     = TO_AFFECTS;
    af.type      = gsn_stoneskin;
    af.level     = level;
    af.duration  = level;
    af.location  = APPLY_AC;
    af.modifier  = -40;
    af.bitvector = 0;
    affect_to_char( victim, &af );

    af.modifier  = PERC_RES;
    af.location  = APPLY_RESIST_WEAPON;
    affect_to_char( victim, &af );

    act( "$n's skin turns to stone.", victim, NULL, NULL, TO_ROOM );
    send_to_char( "Your skin turns to stone.\n\r", victim );
    return TRUE;
}


bool spell_stonetomud( int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    if (is_affected(ch, gsn_stonetomud))
    {
        send_to_char("You have turned the ground to mud too recently.\n", ch);
        return false;
    }

    if (ch->in_room == NULL || !ON_GROUND(ch))
    {
        send_to_char("There's no solid ground here!\n", ch);
        return false;
    }

    if (room_is_affected(ch->in_room, gsn_stonetomud))
    {
        send_to_char("This place has already been turned to mud!\n", ch);
        return false;
    }

    // Apply effect
    AFFECT_DATA af = {0};
    af.where        = TO_ROOM_AFF;
    af.type         = gsn_stonetomud;
    af.level        = level;
    af.duration     = 4 * (check_durablemagics(*ch) ? 2 : 1);
    affect_to_room(ch->in_room, &af);

    // Apply cooldown
    int skill(get_skill(ch, sn)); // Use sn instead of gsn_stonetomud to support rocktomud
    af.type         = sn;
    af.duration     = 40 - (UMAX(0, skill - 70) / 3);
    affect_to_char(ch, &af);

    act("With a sickening lurch, the ground turns to mud!", ch, NULL, NULL, TO_ALL);
    return true;
}

bool spell_strengthen( int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
OBJ_DATA *item;
AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

        item = (OBJ_DATA *) vo;

        if (item == NULL)
        {
        send_to_char("You can't find any such item.\n\r", ch);
        return FALSE;
        }

        if (obj_is_affected(item, sn))
        {
        send_to_char("That item is already strengthened.\n\r", ch);
        return FALSE;
        }
        af.where     = TO_OBJECT;
        af.type      = sn;
        af.level     = level;
        af.duration  = level;
        af.location  = 0;
        af.modifier  = 0;
        af.bitvector = ITEM_NODESTROY;
        affect_to_obj(item, &af);

        act("$n utters softly, and $p looks stronger.", ch, item, NULL, TO_ROOM);
        act("You utter softly, and $p looks stronger.", ch, item, NULL, TO_CHAR);
    return TRUE;
}

bool spell_voiceoftheearth( int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
CHAR_DATA *victim;
ROOM_INDEX_DATA *orig_room;

    if (ch->in_room == NULL)
        return FALSE;

    if (ch->in_room->sector_type == SECT_AIR || ch->in_room->sector_type == SECT_WATER_NOSWIM || ch->in_room->sector_type == SECT_WATER_SWIM || ch->in_room->sector_type == SECT_UNDERWATER)
        {
        act("You cannot speak to the earth from here.", ch, NULL, NULL, TO_CHAR);
        return FALSE;
        }

    if (( victim = get_char_world(ch, target_name)) == NULL)
        {
        act("The earth quietly whispers to you that it cannot find the one you seek.", ch, NULL, NULL, TO_CHAR);
        return TRUE;
        }


    if (victim->in_room == NULL || is_symbol_present(*victim->in_room, OBJ_VNUM_SYMBOL_PROTECT))
    {
        act("The earth whispers to you that it cannot find the one you seek.", ch, NULL, NULL, TO_CHAR);
        return TRUE;
    }
    
    if (victim->in_room->sector_type == SECT_AIR || victim->in_room->sector_type == SECT_WATER_NOSWIM || victim->in_room->sector_type == SECT_WATER_SWIM || victim->in_room->sector_type == SECT_UNDERWATER || (victim->in_room->clan == clan_lookup("SHUNNED") && victim->clan == clan_lookup("SHUNNED")))
        {
        act("The earth whispers to you that it cannot find the one you seek.", ch, NULL, NULL, TO_CHAR);
        return TRUE;
        }

	if ((victim->in_room == ch->in_room) && (victim->in_room->sector_type == SECT_UNDERGROUND))
		{
			do_look(ch,"auto");
			return TRUE;
		}
    if (victim->in_room->sector_type == SECT_UNDERGROUND)

        {
        orig_room = ch->in_room;
	global_bool_ranged_attack = TRUE;
        char_from_room(ch);
        char_to_room(ch, victim->in_room);
        do_look(ch, "auto");
        char_from_room(ch);
        char_to_room(ch, orig_room);
	global_bool_ranged_attack = FALSE;
        return TRUE;
        }

        if (victim->in_room->sector_type == SECT_HILLS)
                act("The earth whispers that the person you seek is in the hills.", ch, NULL, NULL, TO_CHAR);
        else if (victim->in_room->sector_type == SECT_MOUNTAIN)
                act("The earth whispers that the person you seek is in the mountains.", ch, NULL, NULL, TO_CHAR);
        else if (victim->in_room->sector_type == SECT_INSIDE)
                act("The earth whispers that the person you seek is indoors.", ch, NULL, NULL, TO_CHAR);
        else if (victim->in_room->sector_type == SECT_FOREST)
                act("The earth whispers that the person you seek is in the forest.", ch, NULL, NULL, TO_CHAR);
        else if (victim->in_room->sector_type == SECT_SWAMP)
                act("The earth whispers that the person you seek is in the swamp.", ch, NULL, NULL, TO_CHAR);
        else if (victim->in_room->sector_type == SECT_FIELD)
                act("The earth whispers that the person you seek is in the fields.", ch, NULL, NULL, TO_CHAR);
        else if (victim->in_room->sector_type == SECT_CITY)
                act("The earth whispers that the person you seek is in the city.", ch, NULL, NULL, TO_CHAR);
        else if (victim->in_room->sector_type == SECT_DESERT)
                act("The earth whispers that the person you seek is in the desert.", ch, NULL, NULL, TO_CHAR);
        else
                act("The earth whispers that the person you seek cannot be found.", ch, NULL, NULL, TO_CHAR);

    return TRUE;
}


bool spell_wallofstone( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
    int direction;
    EXIT_DATA *pexit;

    if (!ch->in_room)
        return FALSE;

    if (is_affected(ch, gsn_wallofstone))
    {
        send_to_char("You have summoned a wall of stone too recently to call another.\n\r", ch);
        return FALSE;
    }

    if (target_name[0] == '\0')
    {
        send_to_char("Create a wall of stone in which direction?\n\r", ch);
        return FALSE;
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
        send_to_char("There's no exit there to block!\n\r", ch);
        return FALSE;
    }

    if (IS_SET(ch->in_room->exit[direction]->exit_info, EX_WALLED))
    {
        send_to_char("There's already a wall of stone blocking that!\n\r", ch);
        return FALSE;
    }

    create_wallofstone(ch, *ch->in_room, static_cast<Direction::Value>(direction), level);

    AFFECT_DATA af = {0};
    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = 24;
    affect_to_char( ch, &af );
    return true;
}

bool spell_jawsofthemountain(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    int gdam=0,cdam=0;
    bool stalag=TRUE, stalac=TRUE;
    if (ch->in_room->sector_type != SECT_UNDERGROUND)
    {
	send_to_char("You can't summon the jaws of the mountain here.\n\r", ch);
	return FALSE;
    }
     
    if (ch->in_room->exit[DIR_DOWN] != NULL)
        stalag = FALSE;
    else
    {
        gdam = dice(level,3);
        if (saves_spell(level,ch, victim,DAM_PIERCE))
    	    gdam /= 2;
	    if (is_flying(victim))
	        gdam /= 2;
    	if (!saves_spell(level,ch, victim,DAM_PIERCE))
	        victim->move -= gdam/5;
    }
    if (ch->in_room->exit[DIR_UP] != NULL)
    	stalac = FALSE;
    else
    {
	cdam = dice(level,3);
	if (saves_spell(level,ch,victim,DAM_PIERCE))
	    cdam /= 2;
    }        
    
    if (stalag && stalac)
    {
	act("You summon stalagmites and stalactites to pierce $N!",ch,NULL,victim,TO_CHAR);
	act("$n summons stalagmites and stalactites to pierce $N!",ch,NULL,victim,TO_NOTVICT);
	act("$N summons stalagmites and stalactites to pierce you!",victim,NULL,ch,TO_CHAR);
	damage(ch,victim,gdam+cdam,sn,DAM_PIERCE,TRUE);
	return TRUE;
    }
    else
    {
	if (stalag)
	{	
	    act("You summon a stalagmite to pierce $N!",ch,NULL,victim,TO_CHAR);
	    act("$n summons a stalagmite to pierce $N!",ch,NULL,victim,TO_NOTVICT);
	    act("$N summons a stalagmite to pierce you!",victim,NULL,ch,TO_CHAR);
	    damage(ch,victim,gdam,sn,DAM_PIERCE,TRUE);
	    return TRUE;
	}
	if (stalac)
	{
	    act("You summon a stalactite to pierce $N!",ch,NULL,victim,TO_CHAR);
	    act("$n summons a stalactite to pierce $N!",ch,NULL,victim,TO_NOTVICT);
	    act("$N summons a stalactite to pierce you!",victim,NULL,ch,TO_CHAR);
	    damage(ch,victim,cdam,sn,DAM_PIERCE,TRUE);
	    return TRUE;
	}
	send_to_char("You can't summon stalagmites or stalactites here.\n\r",ch);
	return FALSE;
    }
}

