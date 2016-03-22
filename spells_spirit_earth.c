#include "spells_spirit_earth.h"
#include "spells_spirit.h"
#include "Weave.h"
#include <vector>

static std::vector<OBJ_DATA*> find_stones_of_power(CHAR_DATA * ch, size_t bailAfter = static_cast<size_t>(-1))
{
    std::vector<OBJ_DATA*> result;
    for (OBJ_DATA * obj(ch->carrying); obj != NULL && result.size() < bailAfter; obj = obj->next_content)
    {
        if (obj->pIndexData->vnum == OBJ_VNUM_STONE_POWER)
            result.push_back(obj);
    }
    return result;
}

bool check_durablemagics(CHAR_DATA & ch)
{
    bool result(number_percent() <= get_skill(&ch, gsn_durablemagics));
    check_improve(&ch, NULL, gsn_durablemagics, result, 8);
    return result;
}

bool spell_abodeofthespirit(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    // Check for cooldown
    for (AFFECT_DATA * paf(get_affect(ch, sn)); paf != NULL; paf = get_affect(ch, sn, paf))
    {
        if (paf->modifier == 0)
        {
            send_to_char("You are not yet ready to establish a new abode for your spirit.\n", ch);
            return false;
        }
    }

    // Check for an order fount here
    if (ch->in_room == NULL || !Weave::HasWeave(*ch->in_room)
    || !ch->in_room->ley_group->HasFount() || ch->in_room->ley_group->FountOrderPower() <= 0)
    {
        send_to_char("You may only establish a new spiritual abode at a nexus of orderly power.\n", ch);
        return false;
    }

    // Check for norecall
    if (IS_SET(ch->in_room->room_flags, ROOM_NO_RECALL))
    {
        send_to_char("You call out to the spirit of this place, but cannot seem to reach it.\n", ch);
        return false;
    }

    // Echoes
    act("You call out to the spirit of this place, and feel it join to your own!", ch, NULL, NULL, TO_CHAR);
    act("$n calls out, and is surrounded briefly by a faint sparkling light.", ch, NULL, NULL, TO_ROOM);
    affect_strip(ch, sn);

    // Add the cooldown
    AFFECT_DATA af = {0};
    af.where    = TO_AFFECTS;
    af.type     = sn;
    af.level    = level;
    af.duration = 360;
    affect_to_char(ch, &af);

    // Add the effect
    af.duration = -1;
    af.modifier = ch->in_room->vnum;
    af.location = APPLY_HIDE;
    affect_to_char(ch, &af);

    // Change the recall point
    ch->recall_old = ch->in_room;
    ch->recall_to = ch->in_room;
    return true;
}

bool spell_mindofsteel(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    // Check for effect already present
    if (is_affected(ch, sn))
    {
        send_to_char("Your mind is already fortified.\n", ch);
        return false;
    }

    // Check for an order fount here
    if (ch->in_room == NULL || !Weave::HasWeave(*ch->in_room)
    || !ch->in_room->ley_group->HasFount() || ch->in_room->ley_group->FountOrderPower() <= 0)
    {
        send_to_char("You do not sense a nexus of orderly power in this place.\n", ch);
        return false;
    }

    // Echo and add the effect
    send_to_char("You draw the orderly magics of this place into your mind, fortifying it.\n", ch);
    AFFECT_DATA af = {0};
    af.where    = TO_AFFECTS;
    af.type     = sn;
    af.level    = level;
    af.duration = 120;
    af.location = APPLY_MANA;
    af.modifier = number_range(level * 2, level * 3);
    affect_to_char(ch, &af);
    ch->mana  = ch->max_mana;

    // Chance of permanent boost
    if (number_bits(8) == 0)
    {
        send_to_char("As the power fills you it hones your mind, reforging it stronger than before!\n", ch);
        int boost(number_range(3, 7));
        ch->max_mana += boost;
        ch->mana += boost;
    }

    return true;
}

bool check_stoneloupe(CHAR_DATA * ch)
{
    if (ch->in_room == NULL || ch->in_room->sector_type != SECT_UNDERGROUND)
        return false;
    
    if (number_percent() <= get_skill(ch, gsn_stoneloupe))
    {
        check_improve(ch, NULL, gsn_stoneloupe, true, 1);
        return true;
    }
    
    check_improve(ch, NULL, gsn_stoneloupe, false, 1);
    return false;
}

bool spell_holyground(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    // Sanity-check
    if (ch->in_room == NULL)
    {
        bug("holy ground called from null room", 0);
        return false;
    }

    // Check for cooldown
    if (is_affected(ch, sn))
    {
        send_to_char("You are not yet ready to annoint this ground.\n", ch);
        return false;
    }

    // Check for effect already present
    if (area_is_affected(ch->in_room->area, sn))
    {
        send_to_char("This area has already been annointed.\n", ch);
        return false;
    }

    // Check for annointing oil
    OBJ_DATA * obj(lookup_obj_extra_flag(ch, ITEM_ANNOINTINGOIL));
    if (obj == NULL)
    {
        send_to_char("You cannot consecrate this ground without holy oil.\n", ch);
        return false;
    }

    // Time to consecrate
    act("You walk in a circle, pouring out $p as you chant softly.", ch, obj, NULL, TO_CHAR);
    act("When the circle is complete, you bend down and touch it with a single finger, empowering it with your will.", ch, NULL, NULL, TO_CHAR);
    act("Your arcane senses tingle as the area seems to glow with faint golden light.", ch, NULL, NULL, TO_CHAR);

    act("$n walks in a circle, pouring out $p as $e chants softly.", ch, obj, NULL, TO_ROOM);
    act("When the circle is complete, $e bends down and touch it with a single finger.", ch, NULL, NULL, TO_ROOM);
    act("A faint tingle creeps up your spine as the area seems suffused with a faint golden light.", ch, NULL, NULL, TO_ROOM);

    // Echo to PCs in the area
    for (DESCRIPTOR_DATA * d(descriptor_list); d != NULL; d = d->next)
    {
        // Filter out PCs in the same room or not in the same area
        if (d->character == NULL || d->character->in_room == NULL || d->character->in_room->area != ch->in_room->area || d->character->in_room == ch->in_room)
            continue;

        send_to_char("A faint tingle creeps up your spine as the area seems suffused with a faint golden light.\n", d->character);
    }


    // Apply the effect to the area and a cooldown to the caster
    AFFECT_DATA af = {0};
    af.where    = TO_AREA;
    af.type     = sn;
    af.level    = level;
    af.duration = 12;
    affect_to_area(ch->in_room->area, &af);

    af.where    = TO_AFFECTS;
    af.duration = 48;
    affect_to_char(ch, &af);

    extract_obj(obj);
    return true;
}

bool spell_crystalsoul(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    // Check for a stone
    std::vector<OBJ_DATA*> stones(find_stones_of_power(ch, 1));
    if (stones.empty())
    {
        send_to_char("Such powerful magics must consume a stone of power, but you are carrying none.\n", ch);
        return false;
    }

    // Check for already affected
    if (is_affected(ch, sn))
    {
        send_to_char("Your soul is already hardened against magic.\n", ch);
        return false;
    }

    // Apply the effect
    AFFECT_DATA af = {0};
    af.where    = TO_AFFECTS;
    af.type     = sn;
    af.level    = level;
    af.duration = 5 + (level / 10) + (check_durablemagics(*ch) ? 6 : 0);
    af.location = APPLY_RESIST_MAGIC;
    af.modifier = (level + 15);
    affect_to_char(ch, &af);

    // Echoes and stone destruction
    act("You draw forth the power of $p, enfolding your soul in protective crystalline magics.", ch, stones[0], NULL, TO_CHAR);
    act("$n holds $p aloft, and it seems to dissolve through and into $m.", ch, stones[0], NULL, TO_ROOM);
    extract_obj(stones[0]);
    return true;
}

bool spell_earthenvessel(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    // Check for cooldown
    if (is_affected(ch, sn))
    {
        send_to_char("You are not yet ready to channel the earth's spirit again.\n", ch);
        return false;
    }

    // Grab stone count and use to determine power
    size_t stoneCount(find_stones_of_power(ch).size());
    int manaCost(UMAX(250, 400 - (stoneCount * 50)));
    int hpBoost(UMIN(1000, 250 + (stoneCount * 250)));

    // Make sure there is enough mana
    if (ch->max_mana < manaCost)
    {
        send_to_char("You lack the power to channel the earth's spirit.\n", ch);
        return false;
    }

    // Send echo
    act("$n's form shimmers faintly in tones of brown and gold.", ch, NULL, NULL, TO_ROOM);
    switch (stoneCount)
    {
        case 0: send_to_char("With effort, you open a channel to the earth's spirit that it might shield you.\n", ch); break;
        case 1: send_to_char("You tap the stone of power, opening a channel to the earth's spirit that it might shield you.\n", ch); break;
        default: send_to_char("You tap into the stones of power, opening a channel to the earth's spirit that it might shield you.\n", ch); break;
    }

    // Apply the effect
    AFFECT_DATA af = {0};
    af.where    = TO_AFFECTS;
    af.type     = sn;
    af.location = APPLY_MANA;
    af.modifier = -manaCost;
    af.duration = 12;
    affect_to_char(ch, &af);

    af.location = APPLY_NONE;
    af.modifier = hpBoost;
    affect_to_char(ch, &af);

    return true;
}
