#include "merc.h"
#include "spells_air_void.h"
#include "spells_void.h"
#include "EchoAffect.h"

bool spell_callraven(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    CHAR_DATA * familiar(call_familiar(*ch, sn, level, MOB_VNUM_FAMILIAR_RAVEN));
    if (familiar == NULL)
        return false;

    act("With a squawk and ruffle of feathers, $n flutters into the room.", familiar, NULL, NULL, TO_ROOM);
    return true;
}

static void apply_feverwinds(CHAR_DATA & ch, int level, AREA_DATA & area)
{
    // Prepare the effect
    AFFECT_DATA af = {0};
    af.where    = TO_AFFECTS;
    af.type     = gsn_fever;
    af.location = APPLY_STR;
    af.modifier = -5;
    af.bitvector = AFF_PLAGUE;
    
    // Walk the vnums for the area
    for (const VNUM_RANGE * range(area.vnums); range != NULL; range = range->next)
    {
        // Walk the vnums in this range
        for (int vnum(range->min_vnum); vnum <= range->max_vnum; ++vnum)
        {
            // Check for a room at this vnum
            ROOM_INDEX_DATA * room(get_room_index(vnum));
            if (room == NULL)
                continue;

            // Walk the people in this room
            for (CHAR_DATA * vch(room->people); vch != NULL; vch = vch->next_in_room)
            {
                // Echo to all, but disqualify some
                send_to_char("A wave of pestilent air washes over you, reeking of disease and decay.\n", vch);
                if (is_safe_spell(&ch, vch, true) || is_same_group(&ch, vch))
                    continue;

                // Adjust level of the spell according to whether outdoors
                if (IS_OUTSIDE(vch)) af.level = level;
                else af.level = UMAX(1, (af.level * 9) / 10);

                // Check for a save
                if (is_affected(vch, gsn_fever) || saves_spell(af.level, &ch, vch, DAM_DISEASE))
                    continue;

                // Did not save, so echo and apply effect
                send_to_char("You feel hot and feverish.\n", vch);
                act("$n looks pale and feverish.", vch, NULL, NULL, TO_ROOM);

                af.duration = 10 + number_range(0, af.level / 5);
                affect_to_char(vch, &af);
            }
        }
    }
}

bool spell_feverwinds(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    // Sanity check
    if (ch->in_room == NULL)
        return false;

    // Check for cooldown
    if (is_affected(ch, sn))
    {
        send_to_char("You aren't ready to summon more feverwinds just yet.\n", ch);
        return false;
    }

    // Hit everything in the area and adjacent areas
    apply_feverwinds(*ch, level, *ch->in_room->area);
    for (ALINK_DATA * alink(alink_first); alink != NULL; alink = alink->next)
    {
        // Apply the feverwinds to all connected areas
        if (alink->a1 == ch->in_room->area) apply_feverwinds(*ch, level, *alink->a2);
        else if (alink->a2 == ch->in_room->area) apply_feverwinds(*ch, level, *alink->a1);
    }

    // Apply cooldown
    int skill(get_skill(ch, sn));
    AFFECT_DATA af = {0};
    af.where    = TO_AFFECTS;
    af.type     = sn;
    af.level    = level;
    af.duration = 30 - UMAX(0, (skill  - 70) / 3);
    affect_to_char(ch, &af);
    return true;
}

bool spell_fatebones(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    // Make sure the object is a trophied hand
    OBJ_DATA * obj(static_cast<OBJ_DATA*>(vo));
    if (obj->pIndexData->vnum != OBJ_VNUM_TROPHY_HAND)
    {
        act("You cannot etch fatebones out of $p!", ch, obj, NULL, TO_CHAR);
        return false;
    }

    // Echoes
    act("You carve into $p, paring back the flesh and plucking out the knucklebones beneath.", ch, obj, NULL, TO_CHAR);
    act("$n carves into $p, paring back the flesh and plucking out the knucklebones beneath.", ch, obj, NULL, TO_ROOM);
    extract_obj(obj);

    // Set up the handler
    struct CallbackHandler
    {
        static bool HandleMove(CHAR_DATA * ch, ROOM_INDEX_DATA *, EchoAffect *, void *) {HandleCancel(ch); return true;}
        static bool HandlePositionChange(CHAR_DATA * ch, int, EchoAffect *, void *) {HandleCancel(ch); return true;}
        static bool HandleCast(CHAR_DATA * ch, int, int, void *, int, EchoAffect *, void *) {HandleCancel(ch); return true;}

        static bool Finish(CHAR_DATA * ch, EchoAffect *, void * tag)
        {
            // Replace any existing effect with a new one
            affect_strip(ch, gsn_fatebones);

            AFFECT_DATA af = {0};
            af.where    = TO_AFFECTS;
            af.type     = gsn_fatebones;
            af.level    = reinterpret_cast<int>(tag);
            af.location = APPLY_LUCK;
            af.duration = 50 + (get_skill(ch, gsn_fatebones) / 2);
            af.modifier = 29 + af.level;
            affect_to_char(ch, &af);
            return false;
        }

        private:
            static void HandleCancel(CHAR_DATA * ch)
            {
                send_to_char("You abandon your spell, letting the bones turn to ash.\n", ch);
                act("$n abandons $s work, letting the bones turn to ash.\n", ch, NULL, NULL, TO_ROOM);
            }
    };

    // Prepare the EchoAffect
    EchoAffect * echoAff(new EchoAffect(1));
    echoAff->SetPositionCallback(&CallbackHandler::HandlePositionChange);
    echoAff->SetMoveCallback(&CallbackHandler::HandleMove);
    echoAff->SetCastCallback(&CallbackHandler::HandleCast);

    // Add in lines
    echoAff->AddLine(NULL,
                    "Working carefully, you etch a small rune into each of the knucklebones, filling them with power.",
                    "Working carefully, $n etches a small rune into each of the knucklebones, which begin to hum faintly.");

    echoAff->AddLine(NULL,
                    "You gather up the fatebones, shaking them loosely in your grip before casting them in a single toss.",
                    "$n gathers up the bones, shaking them loosely in $s grip before casting them in a single toss.");

    echoAff->AddLine(&CallbackHandler::Finish,
                    "You study the bones before they crumble, letting knowledge of your own fortune fill your subconcious mind!",
                    "$n studies the pattern of the knucklebones, which soon crumble into ash.");

    // Finish applying the effect, then add a cooldown
    echoAff->SetTag(reinterpret_cast<void*>(UMIN(level, obj->level)));
    EchoAffect::ApplyToChar(ch, echoAff);
    return true;
}

bool spell_masquerade(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    // Sanity-check
    if (ch->in_room == NULL)
        return false;

    // Check for cooldown
    if (is_affected(ch, sn))
    {
        send_to_char("You are not ready to call down another masquerade yet.\n", ch);
        return false;
    }

    // Check for area already affected
    if (area_is_affected(ch->in_room->area, sn))
    {
        send_to_char("There is already a masquerade going on here!\n", ch);
        return false;
    }

    // Echo to characters in the area
    for (DESCRIPTOR_DATA * d(descriptor_list); d != NULL; d = d->next)
    {
        // Filter out certain characters
        if (d->connected != CON_PLAYING || d->character->in_room == NULL
        ||  d->character->in_room->area != ch->in_room->area)
            continue;

        // Send the echo
        send_to_char("A strange, dusky light sweeps over the area, transforming it before your sight!\n", d->character);
    }

    // Apply effect
    AFFECT_DATA af = {0};
    af.where    = TO_AREA;
    af.type     = sn;
    af.level    = level;
    af.modifier = ch->id;
    af.duration = level / 5;
    affect_to_area(ch->in_room->area, &af);

    // Apply cooldown
    af.where    = TO_AFFECTS;
    af.duration = 60;
    affect_to_char(ch, &af);
    return true;
}

static AFFECT_DATA * has_miasmaofwaning_effect(CHAR_DATA * ch, int location)
{
    for (AFFECT_DATA * paf(get_affect(ch, gsn_miasmaofwaning)); paf != NULL; paf = get_affect(ch, gsn_miasmaofwaning, paf))
    {
        if (paf->location == location)
            return paf;
    }

    return NULL;
}

int has_miasmaofwaning(CHAR_DATA * ch)
{
    AFFECT_DATA * paf(has_miasmaofwaning_effect(ch, APPLY_NONE));
    return (paf == NULL ? -1 : paf->level);
}

static AFFECT_DATA * has_miasmaofwaning_pain(CHAR_DATA * ch)
{
    return has_miasmaofwaning_effect(ch, APPLY_HIT);
}

void update_miasmaofwaning(ROOM_INDEX_DATA * room)
{
    if (room == NULL)
        return;

    // Check for miasma
    int miasmaLevel(-1);
    CHAR_DATA * exempt(NULL);
    for (CHAR_DATA * ch(room->people); ch != NULL; ch = ch->next_in_room)
    {
        int level(has_miasmaofwaning(ch));
        if (level < 0)
            continue;

        if (miasmaLevel < 0) exempt = ch;
        else exempt = NULL;
        miasmaLevel = UMAX(miasmaLevel, level);
    }

    // Handle the case of no one having the miasma; in this case, need to clear out anyone who was affected by it previously
    if (miasmaLevel < 0)
    {
        for (CHAR_DATA * ch(room->people); ch != NULL; ch = ch->next_in_room)
        {
            AFFECT_DATA * miasma(has_miasmaofwaning_pain(ch));
            if (miasma == NULL)
                continue;

            send_to_char("The nausea departs, and you start to feel healthier once more.\n", ch);
            affect_remove(ch, miasma);
        }
        return;
    }

    // At least one person in the room has the miasma, so make sure everyone is suffering from it
    AFFECT_DATA af = {0};
    af.where    = TO_AFFECTS;
    af.type     = gsn_miasmaofwaning;
    af.level    = miasmaLevel;
    af.location = APPLY_HIT;
    af.duration = -1;

    for (CHAR_DATA * ch(room->people); ch != NULL; ch = ch->next_in_room)
    {
        // Check for existing effect
        AFFECT_DATA * miasma(has_miasmaofwaning_pain(ch));
        if (ch == exempt)
        {
            if (miasma != NULL)
            {
                send_to_char("The nausea departs, and you start to feel healthier once more.\n", ch);
                affect_remove(ch, miasma);
            }
            continue;
        }

        if (miasma != NULL)
            continue;

        // Determine modifier
        af.modifier = ch->max_hit / -10;
        if (af.modifier == 0)
            continue;

        // Apply the effect, saving off how much hit the hp took in the point
        send_to_char("A wave of nausea strikes you, and you feel weaker!\n", ch);
        int newMax(ch->max_hit + af.modifier);
        int damage(0);
        if (ch->hit > newMax)
            damage = ch->hit - newMax;

        af.point = reinterpret_cast<void*>(damage);
        affect_to_char(ch, &af);
        ch->hit -= damage;
    }
}

bool spell_miasmaofwaning(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    // Check for already present
    if (has_miasmaofwaning(ch) >= 0)
    {
        send_to_char("You are already surrounded by a debilitating miasma.\n", ch);
        return false;
    }

    // Echoes
    act("A miasma reeking of decay and malignancy slowly fills the air about you.", ch, NULL, NULL, TO_CHAR);
    act("A miasma reeking of decay and malignancy slowly fills the air about $n.", ch, NULL, NULL, TO_ROOM);
    
    // Add the effect
    AFFECT_DATA af = {0};
    af.where    = TO_AFFECTS;
    af.type     = sn;
    af.level    = level;
    af.duration = 24;
    af.location = APPLY_NONE;
    affect_to_char(ch, &af);

    update_miasmaofwaning(ch->in_room);
    return true;
}
