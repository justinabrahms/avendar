#include "spells_spirit_air.h"
#include "interp.h"
#include "EchoAffect.h"
#include "NameMaps.h"
#include "Weave.h"
#include <sstream>

void check_mirrorofsouls(CHAR_DATA & ch, CHAR_DATA & victim)
{
    // Sanity-check
    if (ch.in_room != victim.in_room || victim.in_room == NULL)
        return;

    // Check for the chaotic weave
    int orderPower(Weave::AbsoluteOrderPower(*victim.in_room));
    if (orderPower >= 0)
        return;

    // Check for the skill
    if (number_percent() > get_skill(&victim, gsn_mirrorofsouls) / 2)
    {
        check_improve(&victim, &ch, gsn_mirrorofsouls, false, 6);
        return;
    }

    orderPower = -orderPower;
    check_improve(&victim, &ch, gsn_mirrorofsouls, true, 6);

    // Make the mob
    CHAR_DATA * mirror(create_mobile(get_mob_index(MOB_VNUM_MIRROROFSOULS)));
    mirror->level     = victim.level;
    mirror->max_hit   = 10 + ((victim.level * orderPower) / 2);
    mirror->hit       = mirror->max_hit;
    mirror->damroll   = 10 * orderPower;
    mirror->hitroll   = 5 + (victim.level / 2);
    mirror->damage[0] = 7;
    mirror->damage[1] = 1 + (victim.level / 6);
    mirror->damage[2] = mirror->damroll;
    mirror->armor[0]  = 0 - ((victim.level * 15) / 10);
    mirror->armor[1]  = 0 - ((victim.level * 15) / 10);
    mirror->armor[2]  = 0 - ((victim.level * 15) / 10);
    mirror->armor[3]  = 0 - ((victim.level * 15) / 10);

    // Build the long desc
    free_string(mirror->long_descr);
    std::ostringstream mess;
    mess << PERS(&victim, &victim) << " the " << race_table[victim.race].name << " is ";
    if (is_flying(&victim))
        mess << "flying ";
    mess << "here.\n";
    mirror->long_descr = str_dup(mess.str().c_str());

    // Build the short desc
    free_string(mirror->short_descr);
    if (victim.short_descr[0] != '\0') mirror->short_descr = str_dup(victim.short_descr);
    else mirror->short_descr = str_dup(victim.name);
    
    // Build the name and description
    free_string(mirror->description);
    mirror->description = str_dup(victim.description);
    setName(*mirror, victim.name);

    // Copy some values
    mirror->race = victim.race;
    mirror->sex = victim.sex;

    // Echo, then have the mirror attack the attacker
    char_to_room(mirror, victim.in_room);
    act("You splinter off a ghostly image of yourself, which charges at $N!", &victim, NULL, &ch, TO_CHAR);
    act("$n splinters off a ghostly image of $mself, which charges at you!", &victim, NULL, &ch, TO_VICT);
    act("$n splinters off a ghostly image of $mself, which charges at $N!", &victim, NULL, &ch, TO_NOTVICT);
    multi_hit(mirror, &ch, TYPE_UNDEFINED);
}

bool spell_bewilderment(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    // Check whether the target is already affected
    CHAR_DATA * victim(static_cast<CHAR_DATA*>(vo));
    if (is_affected(victim, sn))
    {
        if (ch == victim) send_to_char("You are already bewildered.\n", ch);
        else act("$N is already bewildered.", ch, NULL, victim, TO_CHAR);
        return false;
    }

    act("You weave a quick illusion, trying to bewilder $N.", ch, NULL, victim, TO_CHAR);
    send_to_char("An odd series of images flicker in and out of your vision.\n", victim);

    // Check for saves
    if (saves_spell(level, ch, victim, DAM_ILLUSION))
    {
        send_to_char("You grow momentarily confused, but shake it off.\n", victim);
        act("$N looks momentarily confused, but seems to shake it off.", ch, NULL, victim, TO_CHAR);
        return true;
    }

    // Apply the effect
    AFFECT_DATA af = {0};
    af.where    = TO_AFFECTS;
    af.type     = sn;
    af.level    = level;
    af.modifier = number_range(1, 5);
    af.duration = 4 + (level / 5);
    affect_to_char(victim, &af);

    send_to_char("You grow bewildered, and begin to doubt your own senses.\n", victim);
    act("$n looks bewildered and $s actions begin to grow hesitant.", victim, NULL, NULL, TO_ROOM);
    return true;
}

static bool is_relocation_allowed(CHAR_DATA * ch)
{
    if (IS_AFFECTED(ch, AFF_CURSE))
        return false;

    if (ch->in_room == NULL)
        return true;

    if (room_is_affected(ch->in_room, gsn_desecration) || IS_SET(ch->in_room->room_flags, ROOM_NO_RECALL)
    ||  area_is_affected(ch->in_room->area, gsn_suppress) || silver_state == 16)
        return false;

    return true;
}

static bool bilocation_points_to(CHAR_DATA * ch, CHAR_DATA * other)
{
    AFFECT_DATA * paf(get_affect(ch, gsn_bilocation));
    return (paf != NULL && paf->modifier == other->id);
}

CHAR_DATA * find_bilocated_body(CHAR_DATA * ch, AFFECT_DATA * paf)
{
    // A modifier of -1 means no other body
    if (paf->modifier == -1)
        return NULL;

    // Check first by point for speed
    CHAR_DATA * body(static_cast<CHAR_DATA*>(paf->point));
    if (body != NULL && bilocation_points_to(body, ch))
        return body;

    // Point didn't work, so check by id
    body = get_char_by_id_any(paf->modifier);
    if (body != NULL && bilocation_points_to(body, ch))
        return body;

    // Neither worked, no body
    return NULL;
}

CHAR_DATA * find_bilocated_body(CHAR_DATA * ch)
{
    AFFECT_DATA * paf(get_affect(ch, gsn_bilocation));
    if (paf == NULL) return NULL;
    return find_bilocated_body(ch, paf);
}

void relocate_consciousness(CHAR_DATA * ch, CHAR_DATA * body)
{
    ch->desc->character = body;
    ch->desc->original  = (ch->desc->original == NULL ? ch : NULL);
    body->desc          = ch->desc;
    ch->desc            = NULL;
    do_look(body, "auto");
}

void do_relocate(CHAR_DATA * ch, char *)
{
    // Check for bilocation
    AFFECT_DATA * paf(get_affect(ch, gsn_bilocation));
    if (paf == NULL)
    {
        send_to_char("Huh?\n", ch);
        return;
    }

    // Find the other body
    CHAR_DATA * body(find_bilocated_body(ch, paf));
    if (body == NULL)
    {
        send_to_char("You have no other body into which to relocate your consciousness.\n", ch);
        return;
    }

    // Check whether a jump is allowed
    if (ch->in_room != body->in_room)
    {
        if (!is_relocation_allowed(ch))
        {
            send_to_char("Your consciousness is trapped by some dark power.\n", ch);
            return;
        }

        if (!is_relocation_allowed(body))
        {
            send_to_char("The path to your other form is barred by some dark power.\n", ch);
            return;
        }
    }

    // Check for mana
    if (ch->mana < 25)
    {
        send_to_char("This form is too weary to shift your consciousness.\n", ch);
        return;
    }

    // Perform the shift
    expend_mana(ch, 25);
    send_to_char("You pull your consciousness across the Weave, lodging it in your other form.\n", body);
    act("$n's expression grows distant and vacant.", ch, NULL, NULL, TO_ROOM);
    relocate_consciousness(ch, body);
    act("$n shakes $s head as if to clear it, and his expression becomes more focused.", body, NULL, NULL, TO_ROOM);
    WAIT_STATE(body, UMAX(body->wait, PULSE_VIOLENCE));
}

bool spell_bilocation(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    // Sanity-check
    if (ch->in_room == NULL)
    {
        bug("Bilocation called from NULL room", 0);
        return false;
    }

    if (IS_NPC(ch) || ch->desc == NULL || ch->desc->original != NULL)
    {
        send_to_char("You cannot channel such magics.\n", ch);
        return false;
    }

    // Check for cooldown
    if (is_affected(ch, sn))
    {
        send_to_char("You are not ready to once again split your consciousness.\n", ch);
        return false;
    }

    struct CallbackHandler
    {
        static bool HandleMoveFrom(CHAR_DATA * ch, ROOM_INDEX_DATA *, EchoAffect *, void *)
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

        static bool FinishSpell(CHAR_DATA * ch, EchoAffect *, void * tag)
        {
            int level(reinterpret_cast<int>(tag));

            // Add the effect
            AFFECT_DATA af = {0};
            af.where    = TO_AFFECTS;
            af.type     = gsn_bilocation;
            af.level    = level;
            af.duration = number_range(50, 100);

            // Create the body and add the effect to it and the caster
            CHAR_DATA * body(create_mobile(get_mob_index(MOB_VNUM_BILOCATIONBODY)));
            af.modifier = ch->id;
            af.point = ch;
            affect_to_char(body, &af);

            af.modifier = body->id;
            af.duration += 24;
            af.point = body;
            affect_to_char(ch, &af);

            // Build the descriptions and names
            free_string(body->long_descr);
            free_string(body->short_descr);
            free_string(body->description);
            
            body->long_descr = str_dup(ch->long_descr);
            body->short_descr = str_dup(ch->short_descr[0] == '\0' ? ch->name : ch->short_descr);
            body->description = str_dup(ch->description);
            setName(*body, ch->name);

            // Copy some other values
            body->race = ch->race;
            body->sex = ch->sex;
            body->exp = ch->exp;
            body->comm = ch->comm;
            body->lines = ch->lines;

            body->max_hit = ch->max_hit;
            body->hit = ch->hit;
            body->max_mana = ch->max_mana;
            body->mana = ch->mana;
            body->max_move = ch->max_move;
            body->move = ch->move;
            body->level = ch->level;
            body->wimpy = ch->wimpy;
            body->dam_type = ch->dam_type;
            body->dam_verb = str_dup(ch->dam_verb[0] == '\0' ? "punch" : ch->dam_verb);
            body->carry_weight = 0;
            body->carry_number = 0;
            body->alignment = ch->alignment;
            body->form = ch->form;
            body->parts = ch->parts;
            body->size = ch->size;
            body->speaking = ch->speaking;

            for (unsigned int i(0); i < MAX_STATS; ++i)
            {
                // Set the stats; use perm_stat for mod_stat since there should be no mods just yet
                body->perm_stat[i] = ch->perm_stat[i];
                body->mod_stat[i] = ch->perm_stat[i];
                body->max_stat[i] = ch->max_stat[i];
            }

            if (ch->prompt != NULL)
                body->prompt = str_dup(ch->prompt);
 
            // Add to the room
            char_to_room(body, ch->in_room);
            return false;
        }

        private:
            static void HandleCancel(CHAR_DATA * ch)
            {
                send_to_char("You abandon the ritual, letting the gathering light dissolve back into nothingness.\n", ch);
                act("$n abandons $s ritual, letting the gathering light dissolve back into nothingness.", ch, NULL, NULL, TO_ROOM);
            }
    };

    act("You begin a soft chant, causing small motes of golden light to swirl about.", ch, NULL, NULL, TO_CHAR);
    act("$n begins a soft chant, causing small motes of golden light to swirl about.", ch, NULL, NULL, TO_ROOM);

    // Build the echo affect
    EchoAffect * echoAff(new EchoAffect(1));
    echoAff->SetMoveCallback(&CallbackHandler::HandleMoveFrom);
    echoAff->SetPositionCallback(&CallbackHandler::HandlePositionChange);
    echoAff->AddLine(NULL,
                    "As your chant continues, the motes of light begin to coalesce into a vaguely humanoid form.",
                    "As $n's chant continues, the motes of light begin to coalesce into a vaguely humanoid form.");

    echoAff->AddLine(NULL,
                    "You twirl your arms about, guiding the glittering light. A head appears, then shoulders, then torso as the form takes on your own shape!",
                    "$n twirls $s arms about, guiding the glittering light. A head appears, then shoulders, then torso as the form takes on $s own shape!");

    echoAff->AddLine(&CallbackHandler::FinishSpell,
                    "You seal the spell with a final word and the light dims in response, revealing a form nearly indistinguishable from your own.",
                    "$n seals the spell with a final word and the light dims in response, revealing a form nearly indistinguishable from $s own.");

    EchoAffect::ApplyToChar(ch, echoAff);
    return true;
}

bool spell_aurora(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    // Check for already affected
    if (is_affected(ch, sn))
    {
        send_to_char("You are already surrounded by a dazzling aurora.\n", ch);
        return false;
    }

    // Add the effect
    AFFECT_DATA af = {0};
    af.where    = TO_AFFECTS;
    af.type     = sn;
    af.level    = level;
    af.duration = 3 + (level >= 50 ? 1 : 0);
    affect_to_char(ch, &af);

    act("You gesticulate, and are suddenly surrounded by a dazzling aura of light.", ch, NULL, NULL, TO_CHAR);
    act("$n gesticulates, and is suddenly surrounded by a dazzling aura of light.", ch, NULL, NULL, TO_ROOM);
    return true;
}

bool spell_diffraction(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    // Sanity-check
    if (ch->in_room == NULL)
    {
        bug("Diffraction called from null room", 0);
        return false;
    }

    send_to_char("You throw your arms wide, scattering searing rays of light in all directions!\n", ch);
    act("$n throws $s arms wide, scattering searing rays of light in all directions!", ch, NULL, NULL, TO_ROOM);

    // Potentially hit everyone in the room
    int baseDam(10 + level * 2);
    CHAR_DATA * victim_next;
    for (CHAR_DATA * victim(ch->in_room->people); victim != NULL; victim = victim_next)
    {
        victim_next = victim->next_in_room;

        // Disqualify certain people
        if (victim == ch || is_same_group(ch, victim) || is_safe_spell(ch, victim, true))
            continue;

        // Calculate damage, allowing a save for half
        int dam(number_range(baseDam / 2, baseDam * 2));
        if (saves_spell(level, ch, victim, DAM_LIGHT))
            dam /= 2;

        damage_old(ch, victim, dam, sn, DAM_LIGHT, true);
    }

    return true;
}
