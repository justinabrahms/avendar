#include "merc.h"
#include "spells_fire.h"

bool spell_soulbrand(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    OBJ_DATA * obj(static_cast<OBJ_DATA*>(vo));

    // Basic checks
    if (obj->item_type != ITEM_WEAPON)
    {
        send_to_char("Only weapons may be soulbranded.\n", ch);
        return false;
    }

    if (obj_is_affected(obj, gsn_soulbrand))
    {
        act("$p has already been soulbranded.", ch, obj, NULL, TO_CHAR);
        return false;
    }

    if (obj->value[3] == DAM_COLD)
    {
        act("Your brand would not overcome the chill surrounding $p.", ch, obj, NULL, TO_CHAR);
        return false;
    }

    if (IS_OBJ_STAT(obj, ITEM_EVIL))
    {
        act("The malevolent aura about $p would interfere with your brand.", ch, obj, NULL, TO_CHAR);
        return false;
    }

    // Apply the effect
    AFFECT_DATA af = {0};
    af.where    = TO_OBJECT;
    af.type     = sn;
    af.level    = level;
    af.duration = 3 + level / 3;
    af.modifier = ch->id;
    affect_to_obj(obj, &af);

    act("You slap $p with an open palm, marking it with a soulbrand linked to your spirit!", ch, obj, NULL, TO_CHAR);
    act("$n slaps an open palm on $p, and a flash of fiery golden light flares out from the impact!", ch, obj, NULL, TO_ROOM);

    return true;
}

bool spell_manaburn(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    CHAR_DATA * victim(static_cast<CHAR_DATA*>(vo));

    act("You gesture, causing a flare of red-gold flames to stretch from you to $N!", ch, NULL, victim, TO_CHAR);
    act("$n gestures, and a flare of red-gold flames stretches from $m to you, burning at your mana!", ch, NULL, victim, TO_VICT);
    act("$n gestures, and a flare of red-gold flames stretches from $m to $N!", ch, NULL, victim, TO_NOTVICT);
    
    // Get the smaller of the mana pools and check for a save
    int reduction(UMIN(ch->mana, victim->mana));
    if (saves_spell(level, ch, victim, DAM_ENERGY))
        reduction /= 2;

    // Take the mana from both
    expend_mana(ch, reduction);
    expend_mana(victim, reduction);
    return true;
}

bool spell_soulfireshield(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA * victim(static_cast<CHAR_DATA*>(vo));

    // Check for same group
    if (!is_same_group(ch, victim))
    {
        send_to_char("You may only cast that on groupmates.\n", ch);
        return false;
    }

    // Check for already in existence
    if (is_affected(victim, sn))
    {
        send_to_char("They are already shielded by soulfire.\n", ch);
        return false;
    }

    // Apply the effect
    AFFECT_DATA af = {0};
    af.where    = TO_AFFECTS;
    af.type     = sn;
    af.level    = level;
    af.duration = level / 6;
    af.modifier = (ch == victim ? 50 : 25);
    affect_to_char(victim, &af);

    send_to_char("You feel a pleasant warmth fill you.\n", victim);
    if (ch != victim)
        act("You shield $N's spirit with soulfire.", ch, NULL, victim, TO_CHAR);

    return true;
}

bool spell_oriflamme(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    // Sanity check
    if (ch->in_room == NULL)
        return false;

    // Cannot double up oriflammes
    if (room_is_affected(ch->in_room, sn))
    {
        send_to_char("This place is already marked with a blazing standard.\n", ch);
        return false;
    }

    // Cooldown check
    if (is_affected(ch, sn))
    {
        send_to_char("You are not yet ready to call forth another oriflamme.\n", ch);
        return false;
    }

    // Sector check
    if (ch->in_room->sector_type == SECT_UNDERWATER)
    {
        send_to_char("You cannot create an oriflamme underwater.\n", ch);
        return false;
    }

    act("With a triumphant shout, you splay your arms wide and call forth a blazing standard!", ch, NULL, NULL, TO_CHAR);
    act("With a triumphant shout, $n splays $s arms wide and calls forth a blazing standard!", ch, NULL, NULL, TO_ROOM);

    for (CHAR_DATA * target(ch->in_room->people); target != NULL; target = target->next_in_room)
    {
        if (is_same_group(ch, target))
            send_to_char("Your heart swells with inspiration as the oriflamme spurs you on to greatness!\n", target);
    }
    
    AFFECT_DATA af = {0};
    af.where    = TO_ROOM;
    af.type     = sn;
    af.level    = level;
    af.duration = level / 8;
    af.point    = ch;
    affect_to_room(ch->in_room, &af);

                                                     
    // Apply cooldown
    af.where    = TO_AFFECTS;
    af.duration = 25 - (get_skill(ch, sn) / 10);
    affect_to_char(ch, &af);
    return true;
}

bool spell_holyflame(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA * victim(static_cast<CHAR_DATA *>(vo));

    // Damage is holy, so adjust for align/karma
    int diceSize(6);
    int karma(effective_karma(*victim));
    if (karma >= BLACKAURA) diceSize = 9;
    else if (karma >= DARKREDAURA) diceSize = 8;
    else if (karma >= REDAURA) diceSize = 7;
    else if (karma >= FAINTREDAURA) diceSize = 6;
    else if (karma >= PALEGOLDENAURA) diceSize = 6;
    else if (karma >= GOLDENAURA) diceSize = 4;
    else if (karma >= BRIGHTGOLDENAURA) diceSize = 2;
    else diceSize = 1;

    // Apply damage
    int dam(dice(level, diceSize));
    if (saves_spell(level, ch, victim, DAM_HOLY))
        dam /= 2;

    damage_old(ch, victim, dice(level, diceSize), sn, DAM_HOLY, true);

    // Check for coronal glow
    if (!saves_spell(level, ch, victim, DAM_FIRE))
        applyCoronalGlow(ch, victim, level, level / 10, level / 2);

    return true;
}
