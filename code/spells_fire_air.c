#include "merc.h"
#include "spells_fire_air.h"
#include "spells_fire.h"
#include "NameMaps.h"
#include <sstream>

bool checkPyrokineticMirrorAttacked(CHAR_DATA * ch, CHAR_DATA * victim)
{
    // Mirrors are always NPCs
    if (!IS_NPC(victim))
        return false;
        
    // Check for the mirror affect
    AFFECT_DATA * mirrorAff(get_affect(victim, gsn_pyrokineticmirror));
    if (mirrorAff == NULL || mirrorAff->modifier != 1)
        return false;
    
    // We will explode on the attacker; check for tracking cancellage
    if (IS_NPC(ch) && ch->tracking != NULL && !strcmp(ch->tracking->name, victim->name))
    {
        // This was an NPC tracking the caster who placed the illusion; if the NPC doesn't save,
        // he will assume this illusion was sufficient or just be confused enough to stop tracking
        if (!saves_spell(victim->level, victim, ch, DAM_ILLUSION))
            ch->tracking = NULL;
    }

    // Explode on the attacker
    act("$N turns translucent, then explodes towards $n in a jet of flame!", ch, NULL, victim, TO_ROOM);
    act("$N turns translucent, then explodes towards you in a jet of flame!", ch, NULL, victim, TO_CHAR);
    sourcelessDamage(ch, "the illusion", dice(4, mirrorAff->level), gsn_pyrokineticmirror, DAM_FIRE);
    extract_char(victim, true);
    return true;
}

bool spell_burningwisp(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    // Check for currently-floating item that cannot be removed
    OBJ_DATA * floating(get_eq_char(ch, WEAR_FLOAT));
    if (floating != NULL && IS_OBJ_STAT(floating, ITEM_NOREMOVE))
    {
        act("You can't remove $p.", ch, floating, NULL, TO_CHAR);
        return false;
    }

    act("You murmur softly, and a small wisp of fire forms between your hands.", ch, NULL, NULL, TO_CHAR);
    act("$n murmurs softly, and a small wisp of fire forms between $s hands.", ch, NULL, NULL, TO_ROOM);

    // Create the wisp
    OBJ_DATA * wisp(create_object(get_obj_index(OBJ_VNUM_BURNINGWISP), 0));
    wisp->level = ch->level;
    wisp->timer = (level / 2);
    obj_to_char(wisp, ch);
    wear_obj(ch, wisp, true, false, false);
    return true;
}

bool spell_heatwave(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    act("You throw your head back and arms wide, and scream a word of power to the heavens!", ch, NULL, NULL, TO_CHAR);
    act("$n throws $s head back and arms wide, and screams a word of power to the heavens!", ch, NULL, NULL, TO_ROOM);

    for (DESCRIPTOR_DATA * d = descriptor_list; d != NULL; d = d->next)
    {
        // Filter out certain characters
        if (d->connected != CON_PLAYING || d->character->in_room == NULL
        ||  d->character->in_room->area != ch->in_room->area)
            continue;

        // Send the echo to the rest, but then filter out the safe characters from the effects
        send_to_char("The temperature around you abruptly increases as a wave of heat sweeps the area!\n", d->character);
        if (is_safe_spell(ch, d->character, FALSE) || d->character == ch || is_same_group(ch, d->character))
            continue;

        // Hit the targets for movement and miniscule damage
        send_to_char("You feel weary as the wave of heat passes through you.\n\r", d->character);
        int movedam = dice(3, level);
        if (saves_spell(level, ch, d->character, DAM_FIRE))
            movedam /= 2;

        d->character->move = UMAX(0, d->character->move - movedam);
        sourcelessDamage(d->character, "The air", number_range(2, 8), sn, DAM_FIRE);

        // Check for a more lingering effect
        if (!saves_spell(level, ch, d->character, DAM_FIRE))
        {
            send_to_char("The wave of heat drains your body!\n", d->character);
            act("$n looks extremely weary.", d->character, NULL, NULL, TO_ROOM);
            AFFECT_DATA af = {0};
            af.type     = sn;
            af.where    = TO_AFFECTS;
            af.duration = 4;
            af.modifier = -1;
            af.location = (number_percent() < 50 ? APPLY_STR : APPLY_DEX);
            af.level    = level;
            affect_to_char(d->character, &af);
        }

        // Check for thirst
        if (!IS_NPC(d->character) && !saves_spell(level, ch, d->character, DAM_FIRE))
        {
            d->character->pcdata->condition[COND_THIRST] = 0;
            send_to_char("The heat seems to dry you from inside out, and you feel suddenly thirsty.\n", d->character);
        }
    }

    return true;
}

bool spell_pyrokineticmirror(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    // Check for cooldown
    if (is_affected(ch, sn))
    {
        send_to_char("You are not yet ready to produce another pyrokinetic mirror.\n", ch);
        return false;
    }
   
    // Add cooldown 
    AFFECT_DATA af = {0};
    af.where    = TO_AFFECTS;
    af.type     = sn;
    af.level    = level;
    af.duration = UMAX(5, 10 - (get_skill(ch, sn) / 20));
    affect_to_char(ch, &af);

    // Create the image and add the effect
    CHAR_DATA * image(create_mobile(get_mob_index(MOB_VNUM_PYROKINETICMIRROR)));
    af.duration = level + 9;
    af.modifier = 1;
    affect_to_char(image, &af);

    // Build the long description
    free_string(image->long_descr);
    std::ostringstream mess;
    mess << PERS(ch, ch) << " the " << race_table[ch->race].name << " is ";
    if (is_flying(ch))
        mess << "flying ";
    mess << "here.\n";
    image->long_descr = str_dup(mess.str().c_str());

    // Build the short description
    free_string(image->short_descr);
    if (ch->short_descr[0] != '\0')
        image->short_descr = str_dup(ch->short_descr);
    else
        image->short_descr = str_dup(ch->name);
    
    // Build the name and description
    free_string(image->description);
    image->description = str_dup(ch->description);
    setName(*image, ch->name);

    // Copy some values
    image->race = ch->race;
    image->sex = ch->sex;

    // Add to the room
    char_to_room(image, ch->in_room);
    act("In a flash of heat and light, a mirror image of $n flickers into existence!", image, NULL, NULL, TO_ROOM);
    return true;
}
