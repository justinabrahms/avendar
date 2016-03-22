#include "merc.h"
#include "spells_spirit.h"

bool spell_stoneofsalyra(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    // Check for cooldown
    if (is_affected(ch, sn))
    {
        send_to_char("You are not yet ready to form another Salyran link.\n", ch);
        return false;
    }

    // Make sure the object is of type sapphire and not already a stone of salyra
    OBJ_DATA * obj(static_cast<OBJ_DATA*>(vo));
    if (obj->material != material_lookup("sapphire"))
    {
        send_to_char("Only sapphires may be so bonded.\n", ch);
        return false;
    }

    if (obj_is_affected(obj, sn))
    {
        act("$p is already supporting such a link, and cannot withstand another.", ch, obj, NULL, TO_CHAR);
        return false;
    }

    // Perform echoes
    act("You murmur softly as you trace a finger over $p, which glimmers faintly in response.", ch, obj, NULL, TO_CHAR);
    act("$n murmurs softly as $e traces a finger over $p, which glimmers faintly in response.", ch, obj, NULL, TO_ROOM);

    // Apply the cooldown and effect
    AFFECT_DATA af = {0};
    af.where    = TO_AFFECTS;
    af.type     = sn;
    af.level    = level;
    af.duration = 30;
    affect_to_char(ch, &af);

    af.where    = TO_OBJECT;
    af.modifier = ch->id;
    af.bitvector = ITEM_HUM;
    affect_to_obj(obj, &af);

    return true;
}

bool spell_balmofthespirit(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    // Sanity check
    if (ch->in_room == NULL)
    {
        bug("Balm of the spirit called from NULL room", 0);
        return false;
    }

    // Check for cooldown
    if (is_affected(ch, sn))
    {
        send_to_char("You are not yet ready to apply another balm.\n", ch);
        return false;
    }

    // Check for ghost
    if (IS_OAFFECTED(ch, AFF_GHOST))
    {
        send_to_char("You can invoke no such power while in this form.\n", ch);
        return false;
    }

    act("You chant quietly, and a soft warm light spreads over your group.", ch, NULL, NULL, TO_CHAR);
    act("$n chants quietly, and a soft warm light spreads over $s group.", ch, NULL, NULL, TO_ROOM);
    
    // Heal the group
    for (CHAR_DATA * victim(ch->in_room->people); victim != NULL; victim = victim->next_in_room)
    {
        if (!is_same_group(ch, victim) || is_an_avatar(victim))
            continue;

        int mod((aura_grade(victim) * -25) + 150);
        victim->hit = UMIN(victim->hit + mod, victim->max_hit);
        victim->mana = UMIN(victim->mana + mod, victim->max_mana);
        victim->move = UMIN(victim->move + (mod / 3), victim->max_move);
        update_pos(victim);
        send_to_char("A warm feeling fills you, soothing both mind and body.\n", victim);
    }

    // Apply a cooldown
    AFFECT_DATA af = {0};
    af.where    = TO_AFFECTS;
    af.type     = sn;
    af.level    = level;
    af.duration = 10;
    affect_to_char(ch, &af);

    return true;
}

bool spell_martyrsshield(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    // Check for spell already up
    if (is_affected(ch, sn))
    {
        send_to_char("You are already surrounded by a martyr's shield.\n", ch);
        return false;
    }

    // Apply the effect
    AFFECT_DATA af = {0};
    af.where    = TO_AFFECTS;
    af.type     = sn;
    af.level    = level;
    af.duration = level * 2;
    affect_to_char(ch, &af);

    // Send echoes
    act("You trace a circle in the air, empowering it with your will.", ch, NULL, NULL, TO_CHAR);
    act("The circle glows with tones of white and gold, then crumbles away into motes of light which stream into you!", ch, NULL, NULL, TO_CHAR);
    act("$n traces a circle in the air with one hand.", ch, NULL, NULL, TO_ROOM);
    act("The circle glows with tones of white and gold, then crumbles away into motes of light which stream into $n!", ch, NULL, NULL, TO_ROOM);
    return true;
}

bool spell_distillmagic(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    // Check for cooldown
    if (is_affected(ch, sn))
    {
        send_to_char("You cannot distill magic again so soon.\n", ch);
        return false;
    }

    // Apply the effect
    AFFECT_DATA af = {0};
    af.where    = TO_AFFECTS;
    af.type     = sn;
    af.level    = level;
    af.duration = 240;
    affect_to_char(ch, &af);

    send_to_char("A misty aura of magic forms about you.\n", ch);
    return true;
}
