#include "merc.h"
#include "spells_void.h"

extern bool check_dodge args((CHAR_DATA *ch, CHAR_DATA *victim, OBJ_DATA *obj)); // from fight.c

bool spell_callfox(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    CHAR_DATA * familiar(call_familiar(*ch, sn, level, MOB_VNUM_FAMILIAR_FOX));
    if (familiar == NULL)
        return false;

    act("In a flash of red fur, $N darts in and settles at your feet.", ch, NULL, familiar, TO_CHAR);
    act("In a flash of red fur, $N darts in and settles at $n's feet.", ch, NULL, familiar, TO_ROOM);
    return true;
}

bool spell_barbsofalthajji(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    // Check for the effect on the victim
    CHAR_DATA * victim(static_cast<CHAR_DATA*>(vo));
    if (is_affected(victim, sn))
    {
        act("$N has already been pierced by the barbs of Althajji.", ch, NULL, victim, TO_CHAR);
        return false;
    }

    // Echo
    send_to_char("You feel a painful sensitivity spread throughout your body.\n", victim);
    act("You draw the barbs of Althajji into $N, rendering both of you sensitive to pain!", ch, NULL, victim, TO_CHAR);

    // Apply effect to victim
    AFFECT_DATA af = {0};
    af.where    = TO_AFFECTS;
    af.type     = sn;
    af.level    = level;
    af.duration = 2 + (level / 26);
    affect_to_char(victim, &af);

    // Apply/update effect on caster
    AFFECT_DATA * paf(get_affect(ch, sn));
    if (paf == NULL) affect_to_char(ch, &af);
    else paf->duration = af.duration;
    return true;
}

bool spell_focusfury(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    // Check for the effect being present
    CHAR_DATA * victim(static_cast<CHAR_DATA*>(vo));
    if (is_affected(victim, sn))
    {
        if (ch == victim) send_to_char("Your fury is already focused.\n", ch);
        else act("$N's fury is already focused.", ch, NULL, victim, TO_CHAR);
        return false;
    }

    // Find the AC frenzy penalty
    AFFECT_DATA * paf;
    for (paf = get_affect(victim, gsn_frenzy); paf != NULL; paf = get_affect(victim, gsn_frenzy, paf))
    {
        if (paf->location == APPLY_AC)
            break;
    }

    // Make sure it was found
    if (paf == NULL)
    {
        if (ch == victim) send_to_char("You are not in a frenzied state.\n", ch);
        else act("$N is not in a frenzied state.", ch, NULL, victim, TO_CHAR);
        return false;
    }

    // Echoes
    if (ch == victim) act("You focus your fury, channeling the rage within you.", ch, NULL, NULL, TO_CHAR);
    else act("You feel your fury focus as you begin to the channel the rage within you.", victim, NULL, NULL, TO_CHAR);
    act("$n's movements grow less wild, more precise.", victim, NULL, NULL, TO_ROOM);

    // Add the effect and remove the AC penalty
    AFFECT_DATA af = {0};
    af.where    = TO_AFFECTS;
    af.type     = sn;
    af.level    = level;
    af.location = APPLY_HIDE;
    af.duration = paf->duration;
    affect_to_char(victim, &af);
    affect_remove(victim, paf);
    return true;
}

bool spell_chillfireshield(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    // Check for cooldown
    if (is_affected(ch, sn))
    {
        send_to_char("You are not ready to summon another chillfire shield yet.\n", ch);
        return false;
    }

    act("$n gesticulates, and a force shield of darkly-burning fire forms about $m, the flames oddly cool.", ch, NULL, NULL, TO_ROOM);
    act("You gesticulate, and a force shield of darkly-burning fire forms about you, the flames oddly cool.", ch, NULL, NULL, TO_CHAR);

    // Apply the effect
    AFFECT_DATA af = {0};
    af.where    = TO_AFFECTS;
    af.type     = sn;
    af.level    = level;
    af.duration = 24;
    affect_to_char(ch, &af);
    return true;
}

bool spell_nightflamelash(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    // Check for cooldown
    if (is_affected(ch, sn))
    {
        send_to_char("You are not ready to call forth another nightflame lash yet.\n", ch);
        return false;
    }

    // Create the lash
    OBJ_DATA * lash = create_object(get_obj_index(OBJ_VNUM_NIGHTFLAMELASH), 0);
    if (lash == NULL)
    {
        bug("Spell: nightflame lash.  Cannot load lash object.", 0);
        send_to_char("A problem has occured. Please contact the gods\n\r", ch);
        return false;
    }

    // Modify the lash
    lash->level    = level;
    lash->timer    = 30;
    lash->value[1] = 2;
    lash->value[2] = (level / 3) + 8;
    obj_to_char(lash, ch);

    // Apply cooldown
    AFFECT_DATA af = {0};
    af.where    = TO_AFFECTS;
    af.type     = sn;
    af.level    = level;
    af.duration = 60;
    affect_to_char(ch, &af);

    // Apply lash effect
    af.where    = TO_OBJECT;
    af.duration = lash->timer;
    af.point    = ch;
    affect_to_obj(lash, &af);

    act("You slash your hand through the air three times in quick succession.", ch, NULL, NULL, TO_CHAR);
    act("$n slashes $s hand through the air three times in quick succession.", ch, NULL, NULL, TO_ROOM);
    act("On the final pass, a shadowy lash forms from nothing in your hand, leaving behind a trail of flame as it cuts through the air!", ch, NULL, NULL, TO_CHAR);
    act("On the final pass, a shadowy lash forms from nothing in $s hand, leaving behind a trail of flame as it cuts through the air!", ch, NULL, NULL, TO_ROOM);

    return true;
}

bool spell_ashesoflogor(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;

    // Check for existing ashes
    if (is_affected(victim, sn))
    {
        act("$N has already been stricken by the ashes of logor.", ch, NULL, victim, TO_CHAR);
        return false;
    }

    // Check for avoidance
    if (check_dodge(ch, victim, NULL))
    {
        act("$n hurls a handful of dark ashes at $N, who leaps nimbly out of the way!", ch, NULL, victim, TO_NOTVICT);
        act("$n hurls a handful of dark ashes at you, but you leap nimbly out of the way!", ch, NULL, victim, TO_VICT);
        act("You hurl a handful of dark ashes at $N, but $E leaps nimbly out of the way!", ch, NULL, victim, TO_CHAR);
        return true;
    }

    // Apply the effect
    AFFECT_DATA af = {0};
    af.where    = TO_AFFECTS;
    af.type     = sn;
    af.level    = level;
    af.duration = 1 + (level / 7);

    act("$n hurls a handful of dark ashes at $N! They cover $S skin, quickly sinking in!", ch, NULL, victim, TO_NOTVICT);
    act("$n hurls a handful of dark ashes at you! They cover your skin, quickly sinking in!", ch, NULL, victim, TO_VICT);
    act("You hurl a handful of dark ashes at $N! They cover $S skin, quickly sinking in!", ch, NULL, victim, TO_CHAR);

    // Check for defilement
    if (!saves_spell(level, ch, victim, DAM_DEFILEMENT))
    {
        send_to_char("You feel unclean.\n", victim);
        act("You sense the defiling magic take hold of $N!", ch, NULL, victim, TO_CHAR);
        af.modifier = 1;

        if (is_affected(victim, gsn_sanctuary))
        {
            affect_strip(victim, gsn_sanctuary);
            act("The white aura around $n flickers and fades.", victim, NULL, NULL, TO_ROOM);
            act("The white aura around you flickers and fades.", victim, NULL, NULL, TO_CHAR);
        }
    }

    affect_to_char(victim, &af);
    return true;
}
