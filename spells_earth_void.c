#include "spells_earth_void.h"
#include "spells_void.h"
#include "spells_fire.h"
#include "fight.h"
#include "magic.h"
#include "EchoAffect.h"
#include <sstream>

bool spell_callserpent(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    CHAR_DATA * familiar(call_familiar(*ch, sn, level, MOB_VNUM_FAMILIAR_SERPENT));
    if (familiar == NULL)
        return false;

    act("$N slithers in, hissing quietly as $E curls up at your feet.", ch, NULL, familiar, TO_CHAR);
    act("$N slithers in, hissing quietly as $E curls up at $n's feet.", ch, NULL, familiar, TO_ROOM);
    return true;
}

static void deal_gravemaw_damage(CHAR_DATA & victim, int level)
{
    // Calculate the damage
    int dam(dice(level, 4) + 15);
    if (saves_spell(level, NULL, &victim, DAM_PIERCE))
        dam /= 2;

    // Deal the damage
    sourcelessDamage(&victim, "the stone maw", dam, gsn_gravemaw, DAM_PIERCE);
    WAIT_STATE(&victim, 3);

    // Check for cursed room
    if (victim.in_room != NULL && !is_affected(&victim, gsn_curse) 
    && (IS_SET(victim.in_room->room_flags, ROOM_NO_RECALL) || room_is_affected(victim.in_room, gsn_curse)) 
    && !saves_spell(level, NULL, &victim, DAM_NEGATIVE))
    {
        // Echos
        send_to_char("The accursed stone of this place surrounds you, leaving you feeling unclean.\n", &victim);
        act("As the cursed stone and soil surround $n, $e looks very uncomfortable.", &victim, NULL, NULL, TO_ROOM);

        // Apply effect
        AFFECT_DATA af = {0};
        af.where    = TO_AFFECTS;
        af.type     = gsn_curse;
        af.level    = level;
        af.duration = (level / 2) + 10;
        af.location = APPLY_HITROLL;
        af.modifier = -level / 8;
        af.bitvector = AFF_CURSE;
        affect_to_char(&victim, &af);

        af.location = APPLY_SAVING_SPELL;
        af.modifier = level / 8;
        affect_to_char(&victim, &af);
    }
}

bool spell_gravemaw(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    // Check for cooldown
    if (is_affected(ch, sn))
    {
        send_to_char("You cannot bind another idcizon yet.\n", ch);
        return false;
    }
    
    // Check whether lesser or greater requested
    bool greater;
    if (target_name[0] == '\0') 
    {
        // Neither lesser nor greater specified, so check for a trigon to determine which to use
        bool proceed;
        if (check_idcizon_binding(ch, proceed))
        {
            if (!proceed)
                return true;

            greater = true;
        }
        else
            greater = false;
    }
    else if (!str_prefix(target_name, "lesser")) 
        greater = false;
    else if (!str_prefix(target_name, "greater")) 
    {
        // Greater requested, so check for a trigon
        bool proceed;
        bool result(check_idcizon_binding(ch, proceed));
        if (!proceed)
        {
            if (!result)
                send_to_char("You cannot summon a greater idcizon without a Trigon of Binding.\n", ch);

            return result;
        }

        greater = true;
    }
    else
    {
        // Invalid argument
        send_to_char("Did you wish to bind a lesser or greater idcizon?\n", ch);
        return false;
    }

    // Echoes and initial damage
    CHAR_DATA * victim(static_cast<CHAR_DATA*>(vo));
    act("You tear a fanged idcizon from beyond the Veil, binding it into the earth itself!", ch, NULL, NULL, TO_CHAR);
    act("The ground buckles, then rips open as huge stone jaws roar up from it, closing on you!", victim, NULL, NULL, TO_CHAR);
    act("The ground buckles, then rips open as huge stone jaws roar up from it, closing on $n!", victim, NULL, NULL, TO_ROOM);
    deal_gravemaw_damage(*victim, level);

    // Set up the echo effect
    struct CallbackHandler
    {
        static bool HandleMove(CHAR_DATA * victim, ROOM_INDEX_DATA *, EchoAffect *, void *) 
        {
            act("With an unsatisfied rumble, the stone jaws sink back into the earth.", victim, NULL, NULL, TO_ALL);
            return true;
        }

        static bool DealDamage(CHAR_DATA * victim, EchoAffect *, void * tag)
        {
            deal_gravemaw_damage(*victim, reinterpret_cast<int>(tag));
            return false;
        }

        static bool Finish(CHAR_DATA * victim, EchoAffect *, void * tag)
        {
            deal_gravemaw_damage(*victim, reinterpret_cast<int>(tag));
            act("Sated, the horrible stone jaws slump back into the earth.", victim, NULL, NULL, TO_ALL);
            return false;
        }
    };

    // Prepare the echo effect
    EchoAffect * echoAff(new EchoAffect(1));
    echoAff->SetMoveCallback(&CallbackHandler::HandleMove);
    echoAff->SetTag(reinterpret_cast<void*>(level));
    const char * victEcho("The stone jaws continue to rend and tear at you!");
    const char * roomEcho("The stone jaws continue to rend and tear at $n!");

    // Determine how many tearings
    int count(number_range(0, 2));
    if (greater)
        count += 4;
    
    for (int i(0); i < count; ++i)
        echoAff->AddLine(&CallbackHandler::DealDamage, victEcho, roomEcho);
    echoAff->AddLine(&CallbackHandler::Finish, victEcho, roomEcho);
    EchoAffect::ApplyToChar(victim, echoAff);

    // Apply cooldown
    AFFECT_DATA af = {0};
    af.where    = TO_AFFECTS;
    af.type     = sn;
    af.level    = level;
    af.duration = 20;
    affect_to_char(ch, &af);

    return true;
}

bool check_baneblade_modify_parry(OBJ_DATA & obj)
{
    AFFECT_DATA * paf(get_obj_affect(&obj, gsn_baneblade));
    return (paf != NULL && paf->modifier >= 13);
}

void check_baneblade_strike(CHAR_DATA & ch, CHAR_DATA & victim, OBJ_DATA & obj)
{
    // Get the effect
    AFFECT_DATA * paf(get_obj_affect(&obj, gsn_baneblade));
    if (paf == NULL)
        return;

    // Calculate the odds of leaving an effect
    int odds(5);
    if (paf->modifier >= 2) odds += 5;
    if (paf->modifier >= 4) odds += 5;
    if (paf->modifier >= 7) odds += 5;
    if (paf->modifier >= 9) odds += 5;
    if (number_percent() > odds)
        return;

    // Will be leaving an effect, so echo
    std::ostringstream mess;
    mess << "An arc of " << ((paf->modifier >= 5) ? "foul, dark" : "dark") << " energy leaps from $p, striking you!";
    act(mess.str().c_str(), &victim, &obj, NULL, TO_CHAR);

    mess.str("");
    mess << "An arc of " << ((paf->modifier >= 5) ? "foul, dark" : "dark") << " energy leaps from $p, striking $n!";
    act(mess.str().c_str(), &victim, &obj, NULL, TO_ROOM);

    // Check for existing effects, stripping them and obtaining the current vuln
    int modifier(0);
    AFFECT_DATA * vuln_next;
    for (AFFECT_DATA * vuln(get_affect(&victim, gsn_baneblade)); vuln != NULL; vuln = vuln_next)
    {
        vuln_next = vuln->next;
        if (vuln->location != 0)
        {
            modifier = vuln->modifier;
            affect_remove(&victim, vuln);
        }
    }

    // Extra echo for first time
    if (modifier == 0)
    {
        act("You feel more vulnerable.", &victim, NULL, NULL, TO_CHAR);
        act("$n looks suddenly uncomfortable.", &victim, NULL, NULL, TO_ROOM);
    }

    // Prepare new effect
    int cap(paf->modifier >= 12 ? -25 : -20);

    AFFECT_DATA af = {0};
    af.where    = TO_AFFECTS;
    af.type     = gsn_baneblade;
    af.level    = paf->level;
    af.modifier = modifier - (paf->modifier >= 11 ? 2 : 1);
    af.modifier = UMAX(af.modifier, cap);
    af.duration = 1;

    // Determine duration
    if (paf->modifier >= 3) ++af.duration;
    if (paf->modifier >= 6) ++af.duration;
    if (paf->modifier >= 8) ++af.duration;

    // Apply effect(s)
    af.location = APPLY_RESIST_NEGATIVE;
    affect_to_char(&victim, &af);

    if (paf->modifier >= 5)
    {
        af.location = APPLY_RESIST_DEFILEMENT;
        affect_to_char(&victim, &af);
    }
}

bool spell_baneblade(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    // Check cooldown
    for (AFFECT_DATA * paf(get_affect(ch, sn)); paf != NULL; paf = get_affect(ch, sn, paf))
    {
        if (paf->location == APPLY_NONE)
        {
            send_to_char("You cannot channel the dark powers again so soon.\n", ch);
            return false;
        }
    }

    // Verify weapon
    OBJ_DATA * obj(static_cast<OBJ_DATA*>(vo));
    if (obj->item_type != ITEM_WEAPON)
    {
        send_to_char("Only weapons can be made into baneblades.\n", ch);
        return false;
    }

    // Check for holy weapons
    if (IS_OBJ_STAT(obj, ITEM_BLESS) || IS_OBJ_STAT(obj, ITEM_ANTI_EVIL))
    {
        act("$p is too holy to be made into a baneblade.", ch, obj, NULL, TO_CHAR);
        return false;
    }

    // Check for shadow weapons
    static const int shadow(material_lookup("shadow"));
    if (shadow < 0)
    {
        bug("Baneblade: Missing shadow material type", 0);
        send_to_char("An error has occurred, please contact the gods.\n", ch);
        return false;
    }

    AFFECT_DATA * paf(get_obj_affect(obj, sn));
    if (obj->material == shadow || (paf != NULL && paf->modifier >= 13))
    {
        act("There is not enough of $p on this plane for your magics to have any effect on it.", ch, obj, NULL, TO_CHAR);
        return false;
    }

    // Echoes
    act("You chant over $p, channeling unholy power into it!", ch, obj, NULL, TO_CHAR);
    act("$n chants over $p, which throbs with dark power.", ch, obj, NULL, TO_ROOM);

    // Apply cooldown
    int skill(get_skill(ch, sn));
    AFFECT_DATA af = {0};
    af.where    = TO_AFFECTS;
    af.level    = level;
    af.type     = sn;
    af.duration = 2640 - UMAX(0, (skill - 60) * 6);
    affect_to_char(ch, &af);

    // Get the current effect
    if (paf == NULL)
    {
        // First time casting
        AFFECT_DATA af = {0};
        af.where    = TO_OBJECT;
        af.level    = level;
        af.type     = sn;
        af.modifier = 1;
        af.duration = -1;
        af.bitvector = ITEM_EVIL|ITEM_DARK|ITEM_ANTI_GOOD;
        affect_to_obj(obj, &af);
    }
    else
    {
        ++paf->modifier;
        if (paf->modifier == 5)
            act("A foul energy courses over $p, leaving it reeking of filth and decay!", ch, obj, NULL, TO_ALL);
        else if (paf->modifier == 10)
        {
            act("A lean, hungry power fills $p, giving it a nervous energy all its own!", ch, obj, NULL, TO_ALL);
            af = *paf;
            af.bitvector |= ITEM_HUM;
            affect_remove_obj(obj, paf);
            affect_to_obj(obj, &af);
            SET_BIT(obj->value[4], WEAPON_VAMPIRIC);
        }
        else if (paf->modifier == 13)
        {
            // Turn into a wraithblade
            act("$p fades into shadow, flickering with fell energy as it takes on a wraithlike appearance!", ch, obj, NULL, TO_ALL);
            std::string longDesc("(Translucent) ");
            copy_string(obj->description, (longDesc + obj->description).c_str());
            obj->material = shadow;
            obj->weight = UMAX(1, obj->weight / 4);

            af = *paf;
            af.bitvector |= ITEM_INVIS|ITEM_MAGIC;
            affect_remove_obj(obj, paf);
            affect_to_obj(obj, &af);
        }
    }

    return true;
}

void do_clockworksoul(CHAR_DATA * ch, char * argument)
{
    // Verify that it makes sense for this char
    if (get_skill(ch, gsn_clockworksoul) <= 0)
    {
        send_to_char("Huh?\n", ch);
        return;
    }

    // Make sure they typed release
    if (str_prefix(argument, "release"))
    {
        send_to_char("Syntax: clockworksoul release\n", ch);
        return;
    }

    // Get the effect
    AFFECT_DATA * paf(get_affect(ch, gsn_clockworksoul));
    if (paf == NULL || paf->modifier <= 0)
    {
        send_to_char("Your clockwork soul has no pent-up force in need of release.\n", ch);
        return;
    }

    // Deal the damage
    send_to_char("You release the coils of your clockwork soul, letting the forces stored within pour out.\n", ch);
    //deal_clockworksoul_damage(*ch, *paf, paf->modifier);
    deal_clockworksoul_damage(*ch, *paf, 100);
}

/// true result if the ch is killed
bool deal_clockworksoul_damage(CHAR_DATA & ch, AFFECT_DATA & paf, int percent)
{
    // Deal the damage
    int amount((paf.modifier * percent) / 100);
    amount = UMAX(5, amount);
    amount = UMIN(amount, paf.modifier);
    paf.modifier -= amount;
    ch.hit -= amount;

    // Echoes
    dam_message(&ch, &ch, amount, gsn_clockworksoul, "stored damage", false);

    // Check for death
    update_pos(&ch);
    if (ch.position != POS_DEAD)
        return false;

    // Death occurred, kill the char off
    act("You collapse, shorn apart by the internal forces!", &ch, NULL, NULL, TO_CHAR);
    act("$n collapses, $s stored forces proving too much for $m!", &ch, NULL, NULL, TO_ROOM);

    std::ostringstream mess;
    mess << ch.name << " just died from clockwork soul";
    wiznet(const_cast<char*>(mess.str().c_str()), NULL, NULL, WIZ_DEATHS, 0, 0);
    log_string(mess.str().c_str());
    raw_kill(&ch);
    return true;
}

int adjust_for_clockworksoul(CHAR_DATA * ch, CHAR_DATA & victim, int totalDamage)
{
    // Check for the skill
    if (totalDamage <= 0 || number_percent() > get_skill(&victim, gsn_clockworksoul))
        return totalDamage;

    // Check for existing effect
    AFFECT_DATA * paf(get_affect(&victim, gsn_clockworksoul));
    if (paf == NULL)
    {
        // Apply effect
        AFFECT_DATA af = {0};
        af.where    = TO_AFFECTS;
        af.type     = gsn_clockworksoul;
        af.level    = victim.level;
        af.location = APPLY_HIDE;
        af.duration = -1;
        affect_to_char(&victim, &af);

        // Get the effect back
        paf = get_affect(&victim, gsn_clockworksoul);
        if (paf == NULL)
        {
            bug("NULL effect for clockwork soul despite having just placed it", 0);
            return totalDamage;
        }
    }

    // Echo if this is the first
    if (paf->modifier <= 0)
    {
        send_to_char("You feel the machinery of your clockwork soul engage, taking on the force of the blow to be metered out later.\n", &victim);
    }

    // Handle improvement and transfer of damage to the soul
    check_improve(&victim, ch, gsn_clockworksoul, true, 12);
    paf->modifier += totalDamage;
    return 0;
}

bool is_clockworkgolem_present(const ROOM_INDEX_DATA & room)
{
    for (CHAR_DATA * ch(room.people); ch != NULL; ch = ch->next_in_room)
    {
        if (IS_NPC(ch) && ch->pIndexData->vnum == MOB_VNUM_CLOCKWORKGOLEM)
            return true;
    }

    return false;
}

bool spell_clockworkgolem(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    // Sanity-check
    if (ch->in_room == NULL)
    {
        send_to_char("You cannot summon a golem here.\n", ch);
        return false;
    }

    // Check for cooldown
    if (is_affected(ch, sn))
    {
        send_to_char("You cannot construct another clockwork golem yet.\n", ch);
        return false;
    }

    // Check for any pets other than clockwork golems; this requires a traversal of the full char list
    for (CHAR_DATA * pet(char_list); pet != NULL; pet = pet->next)
    {
        if (IS_VALID(pet) && IS_NPC(pet) && pet->master == ch && pet->leader == ch
        && pet->pIndexData->vnum != MOB_VNUM_EARTH_ELEMENTAL)
        {
            send_to_char("You already have a loyal follower, and cannot bind more loyalties.\n", ch);
            return false;
        }
    }

    // Echoes
    act("You gesture, and a golem of gears, springs, and clockwork appears in a puff of steam!", ch, NULL, NULL, TO_CHAR);
    act("$n gestures, and a golem of gears, springs, and clockwork appears in a puff of steam!", ch, NULL, NULL, TO_ROOM);

    // Make the golem
    CHAR_DATA * golem(create_mobile(get_mob_index(MOB_VNUM_CLOCKWORKGOLEM)));
    golem->level = level;
    golem->damroll = 3 + (level / 5);
    golem->hitroll = (level * 2) / 3;
    golem->damage[0] = 0;
    golem->damage[1] = 1;
    golem->damage[2] = golem->damroll;
    golem->max_hit  = level * 20;
    golem->hit = golem->max_hit;

    char_to_room(golem, ch->in_room);
    ch->pet = golem;
    golem->master = ch;
    golem->leader = ch;

    // Apply cooldown
    int skill(get_skill(ch, sn));
    AFFECT_DATA af = {0};
    af.where    = TO_AFFECTS;
    af.type     = sn;
    af.level    = level;
    af.duration = 40 - (UMAX(0, skill - 70) / 3);
    affect_to_char(ch, &af);

    return true;
}

bool spell_kaagnsplaguestone(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    // Check for cooldown
    if (is_affected(ch, sn))
    {
        send_to_char("You are not yet ready to contaminate another place.\n", ch);
        return false;
    }

    // Check for valid room
    if (ch->in_room == NULL || !ON_GROUND(ch))
    {
        send_to_char("There is no stone here to contaminate!\n", ch);
        return false;
    }

    // Check for room already affected
    if (room_is_affected(ch->in_room, sn))
    {
        send_to_char("This place has already been diseased.\n", ch);
        return false;
    }

    // Echoes
    act("You press a hand to the ground, letting the dark energy of disease seep into it.", ch, NULL, NULL, TO_CHAR);
    act("$n presses a hand to the ground, which blackens around it!", ch, NULL, NULL, TO_ROOM);

    // Add effect
    AFFECT_DATA af = {0};
    af.where    = TO_ROOM_AFF;
    af.type     = sn;
    af.level    = level;
    af.duration = (level / 6);
    affect_to_room(ch->in_room, &af);

    // Add cooldown
    int skill(get_skill(ch, sn));
    af.where    = TO_AFFECTS;
    af.duration = 12 - (UMAX(0, skill - 70) / 5);
    affect_to_char(ch, &af);

    return true;
}

bool spell_heartofstone(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    // Check for effect already present
    if (is_affected(ch, sn))
    {
        send_to_char("Your heart is already as cold as stone.\n", ch);
        return false;
    }

    // Echoes
    send_to_char("You feel your heart turn cold as stone.\n", ch);
    act("$n's expression grows stony and emotionless.", ch, NULL, NULL, TO_ROOM);

    affect_strip(ch, gsn_zeal);
    affect_strip(ch, gsn_frenzy);

    // Add effects
    AFFECT_DATA af = {0};
    af.where    = TO_AFFECTS;
    af.type     = sn;
    af.level    = level;
    af.duration = 24;
    af.modifier = 3 + (level / 3);
    af.location = APPLY_RESIST_HOLY;
    affect_to_char(ch, &af);

    af.location = APPLY_RESIST_MENTAL;
    affect_to_char(ch, &af);

    af.location = APPLY_RESIST_ILLUSION;
    affect_to_char(ch, &af);

    af.location = APPLY_RESIST_FEAR;
    af.modifier *= 2;
    affect_to_char(ch, &af);

    af.location = APPLY_CHR;
    af.modifier = -2;
    affect_to_char(ch, &af);
    return true;
}
