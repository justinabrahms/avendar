#include "spells_water_void.h"
#include "fight.h"
#include "spells_water.h"
#include "spells_void.h"

bool spell_calltoad(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    CHAR_DATA * familiar(call_familiar(*ch, sn, level, MOB_VNUM_FAMILIAR_TOAD));
    if (familiar == NULL)
        return false;

    act("Droplets of dark moisture condense into $n's fat, warty form!", familiar, NULL, NULL, TO_ROOM);
    return true;
}

bool spell_hemoplague(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    // Check for already having hemoplague
    CHAR_DATA * victim(static_cast<CHAR_DATA*>(vo));
    if (is_affected(victim, sn))
    {
        if (ch == victim) send_to_char("Your blood is already infectious.\n", ch);
        else act("$N's blood is already infectious.", ch, NULL, victim, TO_CHAR);
        return false;
    }

    // Check for groupmate
    if (!is_same_group(ch, victim))
    {
        send_to_char("You may only cast that on a groupmate.\n", ch);
        return false;
    }

    // Echoes
    if (ch == victim) send_to_char("You feel briefly nauseous as you infect your own blood, but recover quickly.\n", ch);
    else send_to_char("You feel briefly nauseous, but it swiftly passes.\n", victim);
    act("$n looks pale and wane for a moment, but swiftly recovers.", victim, NULL, NULL, TO_ROOM);

    // Apply the effect
    int skill(get_skill(ch, sn));
    AFFECT_DATA af = {0};
    af.where    = TO_AFFECTS;
    af.type     = sn;
    af.level    = level;
    af.duration = 24;
    af.location = APPLY_RESIST_DISEASE;
    af.modifier = (level / 2);
    affect_to_char(victim, &af);

    af.location = APPLY_HIT;
    af.modifier = -10 + (UMAX(0, (skill - 80)) / 4);
    affect_to_char(victim, &af);

    return true;
}

void check_maleficinsight_attack(CHAR_DATA * ch, CHAR_DATA * victim, int & level, int & effectLevel)
{
    // Check for skill passing
    if (number_percent() > get_skill(ch, gsn_maleficinsight))
    {
        check_improve(ch, victim, gsn_maleficinsight, false, 3);
        return;
    }

    // Skill check passed
    check_improve(ch, victim, gsn_maleficinsight, true, 3);
    effectLevel += (level / 6);
    level += 2;
}

void check_boonoftheleech(CHAR_DATA * ch, CHAR_DATA * victim, int sn, int level)
{
    // Check for skill passing
    if (number_percent() > get_skill(ch, gsn_boonoftheleech))
    {
        check_improve(ch, victim, gsn_boonoftheleech, false, 3);
        return;
    }

    // Skill check passed
    check_improve(ch, victim, gsn_boonoftheleech, true, 3);
    act("As your dark magics enfold $N, you draw some of $S life into you.", ch, NULL, victim, TO_CHAR);
    send_to_char("You feel some of your life drawn away by the dark magics.\n", victim);

    int dam(dice(4, UMIN(level, victim->level)));
    damage_old(ch, victim, dam / 2, gsn_boonoftheleech, DAM_NEGATIVE, true);
    ch->hit += dam;
    ch->hit = UMIN(ch->hit, ch->max_hit);
}

bool spell_contaminate(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    // Room check
    if (ch->in_room == NULL || !is_water_room(*ch->in_room))
    {
        send_to_char("You may only contaminate bodies of water.\n", ch);
        return false;
    }

    // Cooldown check
    if (is_affected(ch, sn))
    {
        send_to_char("You are not ready to contaminate another body of water just yet.\n", ch);
        return false;
    }

    act("Chanting, you drag your hand slowly through the water, which turns a murky black in its wake!", ch, NULL, NULL, TO_CHAR);
    act("Chanting, $n drags $s hand slowly through the water, which turns a murky black in its wake!", ch, NULL, NULL, TO_ROOM);

    // Apply the effect and cooldown
    AFFECT_DATA af = {0};
    af.where    = TO_ROOM;
    af.type     = sn;
    af.level    = level;
    af.duration = (level / 5);
    affect_to_room(ch->in_room, &af);

    af.duration += 16;
    af.where    = TO_AFFECTS;
    affect_to_char(ch, &af);

    return true;
}

bool spell_darkchillburst(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    // Sanity check
    if (ch->in_room == NULL)
    {
        bug("Darkchill burst called from NULL room", 0);
        return false;
    }

    // Check for cooldown
    for (AFFECT_DATA * paf(get_affect(ch, sn)); paf != NULL; paf = get_affect(ch, sn, paf))
    {
        if (paf->location == APPLY_NONE)
        {
            send_to_char("You are not yet ready to call upon the night's chill again.\n", ch);
            return false;
        }
    }

    act("You snap out a frosty word of power, and feel a burst of chill energy lash out from you!", ch, NULL, NULL, TO_CHAR);
    act("$n snaps out a frosty word of power, and a burst of chill energy lashes out from $m!", ch, NULL, NULL, TO_ROOM);

    // Prepare the damage vector
    std::vector<DamageInfo> damage(2);
    damage[0].type = DAM_COLD;
    damage[1].type = DAM_NEGATIVE;

    // Prepare the base effect
    AFFECT_DATA af = {0};
    af.where    = TO_AFFECTS;
    af.type     = sn;
    af.duration = (level / 8);

    // Hit occupants of the room who aren't grouped
    CHAR_DATA * victim_next;
    for (CHAR_DATA * victim(ch->in_room->people); victim != NULL; victim = victim_next)
    {
        victim_next = victim->next_in_room;

        // Disqualify certain people
        if (victim == ch || is_same_group(ch, victim) || is_safe_spell(ch, victim, true))
            continue;

        // Calculate damage and saves
        damage[0].amount = dice(level, 4);
        damage[1].amount = dice(level, 4);

        int strDam(-level / 12);
        int dexDam(strDam);
        if (saves_spell(level, ch, victim, DAM_COLD)) {dexDam /= 2; damage[0].amount /= 2;}
        if (saves_spell(level, ch, victim, DAM_NEGATIVE)) {strDam /= 2; damage[1].amount /= 2;}
        
        // Apply effects
        af.location = APPLY_STR;
        af.modifier = strDam;
        affect_to_char(victim, &af);

        af.location = APPLY_DEX;
        af.modifier = dexDam;
        affect_to_char(victim, &af);

        send_to_char("You shudder as the night's chill sweeps over you!\n", victim);
        damage_new(ch, victim, damage, sn, true);
    }

    // Apply a cooldown
    af.location = APPLY_NONE;
    af.duration = 14;
    affect_to_char(ch, &af);
    return true;
}
