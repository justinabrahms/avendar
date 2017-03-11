#include "merc.h"
#include "spells_spirit.h"
#include "spells_void.h"
#include <sstream>
#include <vector>

bool spell_callcat(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    CHAR_DATA * familiar(call_familiar(*ch, sn, level, MOB_VNUM_FAMILIAR_CAT));
    if (familiar == NULL)
        return false;

    act("$N slinks cautiously into the room, then sidles up beside you.", ch, NULL, familiar, TO_CHAR);
    act("$N slinks cautiously into the room, then sidles up beside $n.", ch, NULL, familiar, TO_ROOM);
    return true;
}

bool spell_devouressence(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    // If the Multiplier changes, it needs to be updated in magic.c as well
    static const unsigned int Multiplier = 100;

    // Make sure the caster is not targeting self
    CHAR_DATA * victim(static_cast<CHAR_DATA*>(vo));
    if (ch == victim)
    {
        send_to_char("You cannot devour your own spirit.\n", ch);
        return false;
    }

    // Check for NPC
    if (IS_NPC(victim))
    {
        act("$N's essence isn't strong enough to be of use.", ch, NULL, victim, TO_CHAR);
        return false;
    }

    // Check for already devoured
    AFFECT_DATA * paf(get_affect(victim, sn));
    int resistMod(0);
    if (paf != NULL)
    {
        if (paf->modifier < 0)
        {
            act("$N's essence has already been devoured, and will need time to regenerate.", ch, NULL, victim, TO_CHAR);
            return false;
        }

        resistMod = paf->modifier / Multiplier;
    }

    // Check for a save
    if (!is_same_group(ch, victim) && saves_spell(level - resistMod, ch, victim, DAM_NEGATIVE))
    {
        act("You extend dark, questing power towards $N, but $E forces it away with an effort of will!", ch, NULL, victim, TO_CHAR);
        act("You sense a dark, questing power probe at you, but you manage to force it away!", ch, NULL, victim, TO_VICT);
        return true;
    }

    // Echoes
    act("With a force of will, you tear off a portion of $N's essence, feeding your own spirit!", ch, NULL, victim, TO_CHAR);
    act("A strange sense of weakness comes over you, as if a piece of yourself was suddenly ripped away.", ch, NULL, victim, TO_VICT);

    // Prepare effect
    AFFECT_DATA af = {0};
    af.where    = TO_AFFECTS;
    af.type     = sn;
    af.level    = level;
    af.modifier = -Multiplier;
    af.location = APPLY_MANA;
    af.duration = 120;

    // Apply effect to victim
    affect_strip(victim, sn);
    affect_to_char(victim, &af);

    // Strip current effect from ch, factoring it in
    resistMod *= Multiplier;
    paf = get_affect(ch, sn);
    if (paf != NULL)
    {
        resistMod += paf->modifier;
        affect_remove(ch, paf);
    }

    // Apply new effect to ch, unless it is 0 in balance
    af.modifier = Multiplier + resistMod;
    if (af.modifier != 0)
        affect_to_char(ch, &af);
    return true;
}

void do_sensedreamers(CHAR_DATA * ch, char *)
{
    // Check for dream mastery
    int skill(get_skill(ch, gsn_dreammastery));
    if (skill <= 0)
    {
        send_to_char("Huh?\n", ch);
        return;
    }

    // Check for mana
    if (ch->mana < skill_table[gsn_dreammastery].min_mana)
    {
        send_to_char("You are too weary to extend your senses into the dreamworld.\n", ch);
        return;
    }

    // Iterate the pcs
    std::vector<CHAR_DATA *> found;
    for (DESCRIPTOR_DATA * d(descriptor_list); d->next != NULL; d = d->next)
    {
        // Filter out certain characters
        if (d->connected != CON_PLAYING || !can_see(ch, d->character) 
        || d->character == ch || d->character->position > POS_SLEEPING
        || d->character->in_room == NULL)
            continue;

        // Skill check
        if (number_percent() > skill)
        {
            check_improve(ch, NULL, gsn_dreammastery, false, 4);
            continue;
        }

        // Show the PC
        check_improve(ch, NULL, gsn_dreammastery, true, 4);
        found.push_back(d->character);
    }

    // Handle the case of nothing found
    if (found.empty())
        send_to_char("You stretch your senses into the dreamworld, but find nothing of interest.\n", ch);
    else
    {
        send_to_char("You stretch your senses into the dreamworld, and find...\n", ch);

        for (size_t i(0); i < found.size(); ++i)
        {
            std::ostringstream mess;
            mess << found[i]->name << " sleeps in " << found[i]->in_room->name << "\n";
            send_to_char(mess.str().c_str(), ch);
        }
    }

    // Charge lag and mana
    WAIT_STATE(ch, UMAX(ch->wait, skill_table[gsn_dreammastery].beats));
    expend_mana(ch, skill_table[gsn_dreammastery].min_mana);
}

bool spell_drowse(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA * victim(static_cast<CHAR_DATA*>(vo));

    // Check for already affected
    if (is_affected(victim, sn))
    {
        act("$N is already drowsy.", ch, NULL, victim, TO_CHAR);
        return false;
    }

    // Check for nosubdue
    if (IS_NPC(victim) && IS_SET(victim->act, ACT_NOSUBDUE))
    {
        act("$N would never succumb to such magics.", ch, NULL, victim, TO_CHAR);
        return false;
    }

    act("You chant monotonously, your tone bland and soporific.", ch, NULL, NULL, TO_CHAR);
    act("$n chants monotonously, $s tone bland and soporific.", ch, NULL, NULL, TO_ROOM);

    // Check for save unless already sleeping
    if (victim->position > POS_SLEEPING)
    {
        if (saves_spell(level + lethebane_sleep_level_mod(ch, victim), ch, victim, DAM_MENTAL))
        {
            send_to_char("You feel yourself grow drowsy, but shake it off.\n", victim);
            act("$N looks briefly sleepy, but recovers swiftly.", ch, NULL, victim, TO_CHAR);
            return true;
        }

        send_to_char("Your reflexes slow as you feel yourself grow drowsy.\n", victim);
        act("$n looks noticeably sleepier, suddenly struggling to stay awake.", victim, NULL, NULL, TO_ROOM);
    }
    else
    {
        ++victim->ticks_slept;
        send_to_char("Your sleep grows deeper as you fall into true unconsciousness.\n", victim);
        act("You sense your magics wash over $N, binding $M into a deeper sleep.", ch, NULL, victim, TO_CHAR);
    }

    // Apply the effect
    AFFECT_DATA af = {0};
    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = number_range(4, 5);
    af.modifier  = number_range(2, 3);
    af.bitvector = AFF_SLEEP|AFF_SLOW;
    affect_to_char(victim, &af);
    return true;
}

bool spell_dreamstalk(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA * victim(static_cast<CHAR_DATA*>(vo));

    if (victim == ch)
    {
        send_to_char("You hardly need magic to stalk your own dreams.\n", ch);
        return false;
    }

    // Check for already affected
    if (is_affected(victim, sn))
    {
        act("$N can already be stalked in $S dreams.", ch, NULL, victim, TO_CHAR);
        return false;
    }

    // Check for save
    if (saves_spell(level, ch, victim, DAM_ENERGY))
    {
        act("You try to force a channel open to $N's spirit, but $E resists your efforts.", ch, NULL, victim, TO_CHAR);
        act("$n tries to force a channel open to your spirit, but you resist $s efforts.", ch, NULL, victim, TO_VICT);
        return true;
    }

    // Failed the save
    act("You force a channel open to $N's spirit!", ch, NULL, victim, TO_CHAR);
    act("You feel $n force a channel open to your spirit!", ch, NULL, victim, TO_VICT);

    AFFECT_DATA af = {0};
    af.where    = TO_AFFECTS;
    af.type     = sn;
    af.level    = level;
    af.duration = (level / 3);
    af.modifier = ch->id;
    affect_to_char(victim, &af);

    return true;
}

bool spell_parasiticbond(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA * victim(static_cast<CHAR_DATA*>(vo));

    // Check for already affected
    if (is_affected(ch, sn))
    {
        send_to_char("You are not ready to form another parasitic bond yet.\n", ch);
        return false;
    }

    if (is_affected(victim, sn))
    {
        act("$N is already involved in a parasitic relationship.", ch, NULL, victim, TO_CHAR);
        return false;
    }

    // Sanity-check
    if (ch == victim)
    {
        send_to_char("How do you propose to feed off your own energies?\n", ch);
        return false;
    }

    // Check for save unless in same group
    if (!is_same_group(ch, victim) && saves_spell(level, ch, victim, DAM_ENERGY))
    {
        act("You start to leech off $N's energies, but $E shakes off the bond.", ch, NULL, victim, TO_CHAR);
        act("$n starts to leech off your energies, but you shake off the bond.", ch, NULL, victim, TO_VICT);
        return true;
    }

    act("You begin to leech off $N's energies, binding $S soul to yours.", ch, NULL, victim, TO_CHAR);
    act("You feel a binding around your soul as $n begins to leech off your energies.", ch, NULL, victim, TO_VICT);
    act("$N blanches and grows noticeably paler.", ch, NULL, victim, TO_NOTVICT);

    // Apply the effect
    AFFECT_DATA af = {0};
    af.where    = TO_AFFECTS;
    af.type     = sn;
    af.level    = level;
    af.duration = 1 + (level / 10);
    af.modifier = victim->id;
    affect_to_char(ch, &af);

    af.modifier = 0;
    affect_to_char(victim, &af);
    return true;
}
