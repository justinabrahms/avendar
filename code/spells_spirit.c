#if defined(macintosh)
#include <types.h>
#else
#include <sys/types.h>
#endif
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sstream>
#include "merc.h"
#include "magic.h"
#include "skills_chirurgeon.h"
#include "demons.h"
#include "spells_spirit.h"
#include "spells_spirit_earth.h"
#include "spells_fire.h"
#include "spells_void.h"
#include "Weave.h"
#include "EchoAffect.h"
#include "ShadeControl.h"
#include "RoomPath.h"
#include "NameMaps.h"
#include "Drakes.h"

/* External declarations */
DECLARE_DO_FUN(do_say);
DECLARE_DO_FUN(do_autoyell);
DECLARE_DO_FUN(do_relinquish);
DECLARE_DO_FUN(do_look);
DECLARE_DO_FUN(do_emote);

static bool check_can_consecrate(CHAR_DATA * ch, OBJ_DATA * obj);
extern  bool    global_bool_ranged_attack;

int lethebane_sleep_level_mod(CHAR_DATA * ch, CHAR_DATA * victim)
{
    if (number_percent() <= get_skill(victim, gsn_lethebane))
    {
        check_improve(victim, ch, gsn_lethebane, true, 4);
        return -4;
    }
    
    check_improve(victim, ch, gsn_lethebane, false, 4);
    return 0;
}

OBJ_DATA * lookup_obj_extra_flag(CHAR_DATA * ch, int extraFlag, OBJ_DATA * lastObj)
{
    for (OBJ_DATA * obj(lastObj == NULL ? ch->carrying : lastObj->next_content); obj != NULL; obj = obj->next_content)
    {
        // Break out if quintessence is found
        if (IS_OBJ_STAT_EXTRA(obj, extraFlag))
            return obj;
    }

    return NULL;
}

static OBJ_DATA * require_quintessence(CHAR_DATA * ch)
{
    // Try to find some quintessence
    OBJ_DATA * obj(lookup_obj_extra_flag(ch, ITEM_QUINTESSENCE));
    if (obj == NULL)
        send_to_char("Such potent magics require a vessel of Quintessence.\n", ch);

    return obj;
}

static std::vector<OBJ_DATA*> require_quintessence(CHAR_DATA * ch, unsigned int count)
{
    std::vector<OBJ_DATA*> result;
    if (count == 0)
        return result;

    for (OBJ_DATA * obj(lookup_obj_extra_flag(ch, ITEM_QUINTESSENCE)); obj != NULL; obj = lookup_obj_extra_flag(ch, ITEM_QUINTESSENCE, obj))
    {
        result.push_back(obj);
        if (result.size() == count)
            return result;
    }

    // Did not find enough, return an empty set
    std::ostringstream mess;
    mess << "Such potent magics require ";
    switch (count)
    {
        case 1: mess << "a vessel"; break;
        case 2: mess << "two vessels"; break;
        case 3: mess << "three vessels"; break;
        case 4: mess << "four vessels"; break;
        case 5: mess << "five vessels"; break;
        case 6: mess << "six vessels"; break;
        case 7: mess << "seven vessels"; break;
        case 8: mess << "eight vessels"; break;
        case 9: mess << "nine vessels"; break;
        case 10: mess << "ten vessels"; break;
        default: mess << count << " vessels"; break;
    }
    mess << " of Quintessence.\n";
    send_to_char(mess.str().c_str(), ch);
    return std::vector<OBJ_DATA*>();
}

static void consume_quintessence(CHAR_DATA * ch, OBJ_DATA * obj)
{
    // Destroy the quintessence
    act("$p is unmade by the release of its Quintessence.", ch, obj, NULL, TO_CHAR);
    extract_obj(obj);
} 

static void consume_quintessence(CHAR_DATA * ch, const std::vector<OBJ_DATA*> & objs)
{
    for (size_t i(0); i < objs.size(); ++i)
        consume_quintessence(ch, objs[i]);
}

void summon_avenging_seraph(int level, CHAR_DATA * target)
{
    if (target->in_room == NULL)
    {
        bug("Summon avenging seraph called on null room", 0);
        return;
    }

    CHAR_DATA * angel = create_mobile(get_mob_index(MOB_VNUM_AVENGING_SERAPH));
    char_to_room(angel, target->in_room);
    angel->level    = level + 2;
    angel->hit      = level * 20;
    angel->max_hit  = level * 20;
    angel->damroll  = (level * 14) / 10;
    angel->hitroll  = (level * 14) / 10;
    angel->damage[0]= level/5;
    angel->damage[1]= level/8;
    angel->armor[0] = 0 - ((level * 15) / 10);
    angel->armor[1] = 0 - ((level * 15) / 10);
    angel->armor[2] = 0 - ((level * 15) / 10);
    angel->armor[3] = 0 - ((level * 15) / 10);
    multi_hit(angel, target, TYPE_UNDEFINED);
}

bool is_demon(CHAR_DATA * ch)
{
    if (race_lookup("demon") == ch->race || ch->demontrack != NULL || IS_OAFFECTED(ch, AFF_DEMONPOS))
        return true;
    
    if (IS_AFFECTED(ch, AFF_CHARM) && is_affected(ch, gsn_demoniccontrol))
        return true;

    return false;
}

const char * litanyName(LitanyType litany)
{
    switch (litany)
    {
        case Litany_Benediction: return "benediction";
        case Litany_Mortification: return "mortification";
        case Litany_Purification: return "purification";
        default: break;
    }

    bug("Unknown litany type in litantyName", 0);
    return "";
}

bool perform_litany_skill_check(CHAR_DATA * ch, CHAR_DATA * victim, LitanyType litany, int divisor)
{
    AFFECT_DATA * paf(get_affect(ch, gsn_chantlitany));
    if (paf == NULL || paf->modifier != litany)
        return false;

    if (number_percent() <= (get_skill(ch, gsn_chantlitany) / divisor))
    {
        check_improve(ch, victim, gsn_chantlitany, true, 8);
        return true;
    }
    
    check_improve(ch, victim, gsn_chantlitany, false, 8);
    return false;
}

void do_assesssoul(CHAR_DATA * ch, char * argument)
{
    // Check for thanatopsis
    if (!is_affected(ch, gsn_thanatopsis))
    {
        send_to_char("You are not sufficiently in tune with the world to assess a soul.\n", ch);
        return;
    }

    // Check for mana
    if (ch->mana < 120)
    {
        send_to_char("You are too weary to assess a soul.\n", ch);
        return;
    }

    // Get a target
    CHAR_DATA * victim(NULL);
    if (argument[0] == '\0') 
        victim = ch;
    else
    { 
        victim = get_char_room(ch, argument);
        if (victim == NULL)
        {
            send_to_char("You see no one here by that name.\n", ch);
            return;
        }
    }

    // Cannot scrutinize an NPC
    if (IS_NPC(victim))
    {
        send_to_char("You cannot sense the strength of their spirit.\n", ch);
        return;
    }

    std::ostringstream mess;
    if (ch == victim)
    {
        send_to_char("You concentrate for a while on your own spirit, trying to assess its strength.\n", ch);
        mess << "After a time, it becomes clear that your soul";
    }
    else
    {
        act("You stare for a while at $N, and slowly come to realize the strength of $S spirit.", ch, NULL, victim, TO_CHAR);
        act("$n stares at you for a long while, studying you.", ch, NULL, victim, TO_VICT);
        act("$n stares at $N for a long while, studying $M.", ch, NULL, victim, TO_NOTVICT);
        mess << "After a time, it becomes clear that $S soul";
    }

    // Get a percentage of life burned up
    int lifeBurned((100 * victim->pcdata->death_count) / victim->pcdata->max_deaths);
    if (lifeBurned <= 15) mess << " has a very strong connection to this plane.";
    else if (lifeBurned <= 30) mess << " has a strong connection to this plane.";
    else if (lifeBurned <= 45) mess << " has a solid connection to this plane.";
    else if (lifeBurned <= 55) mess << " has a stable connection to this plane.";
    else if (lifeBurned <= 65) mess << "'s hold on this plane is beginning to weaken.";
    else if (lifeBurned <= 75) mess << "'s hold on this plane is frayed and torn, though intact.";
    else if (lifeBurned <= 85) mess << " has a faint and unsteady connection to this plane.";
    else if (lifeBurned <= 95) mess << " is barely tethered to this plane.";
    else mess << " has almost no grip on this plane, and could break free at any moment.";

    act(mess.str().c_str(), ch, NULL, victim, TO_CHAR);
    
    // Hit them for lag and mana
    WAIT_STATE(ch, UMAX(ch->wait, 2 * PULSE_VIOLENCE));
    expend_mana(ch, 120);
}

void do_chantlitany(CHAR_DATA * ch, char * argument)
{
    // Check for skill
    int skill(get_skill(ch, gsn_chantlitany));
    if (skill <= 0)
    {
        send_to_char("Huh?\n", ch);
        return;
    }

    // Check for cooldown
    if (is_affected(ch, gsn_chantlitany))
    {
        send_to_char("It is not time for another litany yet.\n", ch);
        return;
    }

    // Check for which litany was specified
    if (argument[0] == '\0')
    {
        send_to_char("Which litany did you wish to recite?\n", ch);
        return;
    }

    LitanyType litany;
    if (!str_prefix(argument, "benediction")) litany = Litany_Benediction;
    else if (!str_prefix(argument, "mortification")) litany = Litany_Mortification;
    else if (!str_prefix(argument, "purification")) litany = Litany_Purification;
    else
    {
        send_to_char("You are not versed in such a litany.\n", ch);
        return;
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
            // Add the affect
            AFFECT_DATA af = {0};
            af.where    = TO_AFFECTS;
            af.type     = gsn_chantlitany;
            af.level    = ch->level;
            af.modifier = reinterpret_cast<int>(tag);
            af.duration = 240;

            if (af.modifier == Litany_Purification)
                af.bitvector = AFF_DETECT_EVIL|AFF_DETECT_GOOD;

            affect_to_char(ch, &af);
            return false;
        }

        private:
            static void HandleCancel(CHAR_DATA * ch)
            {
                act("You abandon your litany, and feel the power diminish once more.", ch, NULL, NULL, TO_CHAR);
                act("$n abandons $s litany, and the energy about $m vanishes.", ch, NULL, NULL, TO_ROOM);
            }
    };

    // Set up the echo affect
    EchoAffect * echoAff(new EchoAffect(1));
    echoAff->SetTag(reinterpret_cast<void*>(litany));
    echoAff->SetMoveCallback(&CallbackHandler::HandleMoveFrom);
    echoAff->SetPositionCallback(&CallbackHandler::HandlePositionChange);

    // Prepare echoes
    switch (litany)
    {
        case Litany_Benediction:
            act("You begin to chant in a low, soft voice, reciting the opening sequence of the Litany of Benediction.", ch, NULL, NULL, TO_CHAR);
            act("$n begins to chant in a low, soft voice.", ch, NULL, NULL, TO_ROOM);

            echoAff->AddLine(NULL,
                            "As you continue the gentle recitation, you feel your spirit begin to grow more receptive to positive energies.",
                            "As $n continues $s gentle recitation, a faint golden glow seems to suffuse him.");

            echoAff->AddLine(&CallbackHandler::FinishSpell,
                            "You finish the Litany of Benediction, and realize your whole body is glowing with a gentle golden light!",
                            "$n slowly stops chanting as $s whole body glows with a gentle golden light!");
            break;

        case Litany_Mortification:
            act("You begin to chant in a loud, sonorous tone, the words of the Litany of Mortification echoing through the air.", ch, NULL, NULL, TO_CHAR);
            act("$n begins to chant in a loud, sonorous tone, $s words echoing through the air.", ch, NULL, NULL, TO_ROOM);

            echoAff->AddLine(NULL,
                            "As your voice carries, bands of harsh golden energy laced with silver form in front of you.",
                            "As $n's voice carries, bands of harsh golden energy laced with silver form in front of $m.");

            echoAff->AddLine(&CallbackHandler::FinishSpell,
                            "You end your recitation abruptly, and feel the righteous energy surge into you!",
                            "$n ends $s recitation abruptly, and the harsh energy appears to collapse into $m!");
            break;

        case Litany_Purification:
            act("You chant slowly, your voice quiet but growing louder as you speak the first phrases of the Litany of Purification.", ch, NULL, NULL, TO_CHAR);
            act("$n chants slowly, $s voice quiet but growing louder as $s speaks.", ch, NULL, NULL, TO_ROOM);

            echoAff->AddLine(NULL,
                            "As the volume of your litany increases, a pure white light dawns overhead, shining down upon you.",
                            "As the volume of $n's litany increases, a pure white light dawns overhead, shining down upon $m.");

            echoAff->AddLine(&CallbackHandler::FinishSpell,
                            "You gradually cease your chanting, basking in the holy light as you finish the Litany of Purification.",
                            "$n gradually ceases $s chanting, and slowly the pure white light fades.");
            break;

        default: break;
    }

    // Handle lag and mana expenditure
    WAIT_STATE(ch, UMAX(ch->wait, skill_table[gsn_chantlitany].beats));
    expend_mana(ch, skill_table[gsn_chantlitany].min_mana);
    
    EchoAffect::ApplyToChar(ch, echoAff);
}

void do_triumphantshout(CHAR_DATA * ch, char * argument)
{
    // Sanity-check
    if (ch->in_room == NULL)
    {
        bug("triumphant shout called from NULL room", 0);
        return;
    }

    // Check whether the character knows this skill
    int skill(get_skill(ch, gsn_triumphantshout));
    if (skill <= 0)
    {
        send_to_char("Huh?\n", ch);
        return;
    }

    // Check for cooldown
    if (is_affected(ch, gsn_triumphantshout))
    {
        send_to_char("You are already spurred on by a triumphant shout!\n", ch);
        return;
    }

    // Check for unacceptable forms of avatar
    int avatarType(type_of_avatar(ch));
    if (avatarType != -1 && avatarType != gsn_avatarofthelodestar)
    {
        send_to_char("You cannot speak in this form, even to shout.\n", ch);
        return;
    }

    // Check for fighting
    if (ch->position != POS_FIGHTING)
    {
        send_to_char("Shouting triumphantly when not fighting is in poor taste.\n", ch);
        return;
    }

    act("You shout triumphantly, charging further into the fray!", ch, NULL, NULL, TO_CHAR);
    act("$n shouts triumphantly, charging further into the fray!", ch, NULL, NULL, TO_ROOM);
    int duration(number_range(1, 2));

    // Apply the effect to groupmates (if already present, just reset the modifier)
    for (CHAR_DATA * gch(ch->in_room->people); gch != NULL; gch = gch->next_in_room)
    {
        // Filter out the non-groupmates and wallflowers
        if (gch->position != POS_FIGHTING || !is_same_group(ch, gch))
            continue;

        send_to_char("Your heart pounds with excitement, and you lash out with renewed vigor!\n", gch);

        // Check for already existing
        AFFECT_DATA * paf(get_affect(gch, gsn_triumphantshout));
        if (paf == NULL)
        {
            // Apply the effect
            AFFECT_DATA af = {0};
            af.where    = TO_AFFECTS;
            af.type     = gsn_triumphantshout;
            af.level    = ch->level;
            af.duration = duration;
            af.modifier = number_range(2, 4);
            affect_to_char(gch, &af);
        }
        else
        {
            // Already exists, just update the modifier and duration
            int modifier(number_range(2, 4));
            paf->modifier = UMAX(paf->modifier, modifier);
            paf->duration = UMAX(paf->duration, duration);
        }
    }

    // Handle lag and mana expenditure
    WAIT_STATE(ch, UMAX(ch->wait, skill_table[gsn_triumphantshout].beats));
    expend_mana(ch, skill_table[gsn_triumphantshout].min_mana);
}

const char * auraBaseName(SpiritAura aura)
{
    switch (aura)
    {
        case Aura_Soothing: return "soothing";
        case Aura_Empathy:  return "empathy";
        case Aura_Genesis:  return "genesis";
    }

    bug("auraBaseName: unknown aura", 0);
    return "???";
}

static const char * auraName(AFFECT_DATA * paf)
{
    switch (paf->modifier)
    {
        case Aura_Soothing: return "a cool, comforting aura";
        case Aura_Empathy:  return "a clear warm aura";
        case Aura_Genesis:  return "a light golden aura";
    }

    bug("auraName: Unknown modifier for radiate aura", 0);
    return "";
}

static void echoAura(CHAR_DATA * ch, AFFECT_DATA * paf, const char * verb)
{
    std::ostringstream mess;
    mess << "You " << verb << " radiating " << auraName(paf) << ".\n";
    send_to_char(mess.str().c_str(), ch);

    mess.str("");
    mess << "$n " << verb << "s radiating " << auraName(paf) << ".";
    act(mess.str().c_str(), ch, NULL, NULL, TO_ROOM);
}

void do_radiateaura(CHAR_DATA * ch, char * argument)
{
    // Check whether the character knows this skill
    int skill(get_skill(ch, gsn_radiateaura));
    if (skill <= 0)
    {
        send_to_char("Huh?\n", ch);
        return;
    }

    // Check that an argument was supplied
    if (argument[0] == '\0')
    {
        send_to_char("Radiate which aura?\n", ch);
        return;
    }

    // Get any existing aura, then determine which aura to radiate
    AFFECT_DATA * paf(get_affect(ch, gsn_radiateaura));
    if (!str_prefix(argument, "none"))    
    {
        if (paf == NULL)
            send_to_char("You are already not radiating an aura.\n", ch);
        else
        {
            echoAura(ch, paf, "cease");
            affect_remove(ch, paf);
        }

        return;
    }

    SpiritAura newAura;
    if (!str_prefix(argument, "soothing")) newAura = Aura_Soothing;
    else if (!str_prefix(argument, "empathy")) newAura = Aura_Empathy;
    else if (!str_prefix(argument, "genesis")) newAura = Aura_Genesis;
    else
    {
        send_to_char("You do not know how to radiate such an aura.\n", ch);
        return;
    }

    // Make sure they aren't just radiating the same aura
    if (paf != NULL && paf->modifier == newAura)
    {
        std::ostringstream mess;
        mess << "You are already radiating an aura of " << auraBaseName(newAura) << ".\n";
        send_to_char(mess.str().c_str(), ch);
        return;
    }

    WAIT_STATE(ch, UMAX(ch->wait, skill_table[gsn_radiateaura].beats));
    expend_mana(ch, skill_table[gsn_radiateaura].min_mana);

    // Skill check
    if (number_percent() > skill)
    {
        send_to_char("You try to radiate a new aura, but cannot channel the energy properly.\n", ch);
        check_improve(ch, NULL, gsn_radiateaura, false, 2);
        return;
    }
    check_improve(ch, NULL, gsn_radiateaura, true, 2);

    // Switch to the new aura
    if (paf == NULL)
    {
        // Apply the effect
        AFFECT_DATA af = {0};
        af.where    = TO_AFFECTS;
        af.type     = gsn_radiateaura;
        af.level    = ch->level;
        af.duration = -1;
        af.modifier = newAura;
        affect_to_char(ch, &af);
        echoAura(ch, &af, "begin");
        return;
    }

    // Just switch the effect and update level
    echoAura(ch, paf, "cease");
    paf->level = ch->level;
    paf->modifier = newAura;
    echoAura(ch, paf, "begin");
}

bool spell_spectrallantern(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    // Sanity-check
    if (ch->in_room == NULL)
    {
        bug("Spectral lantern called from null room", 0);
        return false;
    }

    // Check for cooldown
    if (is_affected(ch, sn))
    {
        send_to_char("You are not ready to create another spectral lantern just yet.\n", ch);
        return false;
    }

    if (room_is_affected(ch->in_room, sn))
    {
        send_to_char("A spectral light already guides spirits here.\n", ch);
        return false;
    }

    // Get some quintessence
    OBJ_DATA * quintessence(require_quintessence(ch));
    if (quintessence == NULL)
        return false;

    // Add the effect to the room and character
    AFFECT_DATA af = {0};
    af.where    = TO_ROOM;
    af.type     = sn;
    af.level    = level;
    af.duration = 120;
    affect_to_room(ch->in_room, &af);

    af.where    = TO_AFFECTS;
    affect_to_char(ch, &af);

    // Echo to the caster and room
    act("You cup your hands together and whisper into them, causing a small silver light to form between them.", ch, NULL, NULL, TO_CHAR);
    act("As the glow forms into a pale silver sphere, you step back from it, letting its spectral light gleam unhindered.", ch, NULL, NULL, TO_CHAR);
    act("$n cups $s hands together and whispers into them, causing a small silver light to form between them.", ch, NULL, NULL, TO_ROOM);
    act("As the glow forms into a pale silver sphere, $n steps back from it, letting its spectral light gleam unhindered.", ch, NULL, NULL, TO_ROOM);

    // Update the shade paths
    ShadeControl::UpdateShadePathing();

    // Consume the quintessence    
    consume_quintessence(ch, quintessence);
    return true;
}

bool spell_shadeswarm(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    CHAR_DATA * victim(static_cast<CHAR_DATA*>(vo));

    // Sanity-check
    if (ch->in_room == NULL)
    {
        bug("Shadeswarm called from null room", 0);
        return false;
    }

    // Cannot swarm yourself
    if (ch == victim)
    {
        send_to_char("Only the mad would wish that upon themselves.\n", ch);
        return false;
    }

    // Cannot swarm the same person twice
    if (is_affected(victim, sn))
    {
        act("$E is already haunted by lost souls.", ch, NULL, victim, TO_CHAR);
        return false;
    }

    // Check for a circle of protection
    if (is_symbol_present(*ch->in_room, OBJ_VNUM_SYMBOL_PROTECT))
    {
        act("You cannot marshal shades with the Circle warding them away.", ch, NULL, NULL, TO_CHAR);
        return false;
    }

    // Check for shades in the room, figuring out which ones will respond
    bool anyFound(false);
    std::vector<CHAR_DATA*> shades;
    int caster_grade(aura_grade(ch));
    for (CHAR_DATA * shade(ch->in_room->people); shade != NULL; shade = shade->next_in_room)
    {
        // Filter out non-shades
        if (!IS_NPC(shade) || !IS_SET(shade->nact, ACT_SHADE))
            continue;

        // Determine whether this shade will participate
        anyFound = true;
        if (IS_EVIL(shade))
        {
            // Evil shades will always participate in a good swarming
            shades.push_back(shade);
            continue;
        }
        
        int saveMod(get_curr_stat(ch, STAT_CHR) - 20);
        if (IS_GOOD(shade))
        {
            // Good shades will always haunt evils
            int victim_grade(aura_grade(victim));
            if (victim_grade > 0)
            {
                shades.push_back(shade);
                continue;
            }

            // Target is not evil, so now the good shade has to decide who to trust
            // Each aura differential between caster and target is worth effective levels in the save below
            saveMod = (victim_grade - caster_grade) * 5;
        }

        // To get a good shade to attack a non-evil or a neutral shade to attack anybody requires you to bypass their saves
        if (!saves_spell(level + saveMod, ch, shade, DAM_CHARM))
            shades.push_back(shade);
    }

    // Make sure there was at least one shade here
    if (!anyFound)
    {
        send_to_char("There are no shades here to call.\n", ch);
        return false;
    }

    act("You let out a keening wail, calling to the spirit world to swarm $N!", ch, NULL, victim, TO_CHAR);
    act("$n throws back $s head and lets out a keening wail touched with arcane power!", ch, NULL, victim, TO_ROOM);

    // Make sure at least one shade is ready to swarm
    if (shades.empty())
    {
        send_to_char("Nothing from the spirit realm responds to your call.\n", ch);
        return true;
    }

    // Swarm the target
    int power(0);
    bool canSeeShades(false);
    for (size_t i(0); i < shades.size(); ++i)
    {
        std::ostringstream mess;
        mess << shades[i]->short_descr << " flies at $N, weaving through and around $S body before finally settling inside, wailing all the while!";
        act(mess.str().c_str(), ch, NULL, victim, TO_CHAR);

        for (CHAR_DATA * echoChar(ch->in_room->people); echoChar != NULL; echoChar = echoChar->next_in_room)
        {
            if (!can_see(echoChar, shades[i]) || echoChar == ch)
                continue;
            
            if (echoChar == victim)
                act("$N suddenly wails and flies at you, weaving through you before settling inside!", victim, NULL, shades[i], TO_CHAR);
            else
                act("A shade suddenly wails, diving towards $N and vanishing within!", echoChar, NULL, victim, TO_CHAR);
        }

        power += (shades[i]->level * shades[i]->timer);
        extract_char(shades[i], TRUE);
    }

    // Echo to the victim
    if (canSeeShades && !saves_spell(level, ch, victim, DAM_FEAR))
    {
        send_to_char("You reel in panic!\n", ch);
        if (number_bits(2) == 0)
            do_flee(victim, "");

        WAIT_STATE(victim, UMAX(victim->wait, PULSE_VIOLENCE));
    }
    else
        send_to_char("An unpleasant chill creeps down your spine, spreading to your extremities as you realize you are haunted!\n", victim);

    // Check for a save to halve the power
    power /= 10;
    if (saves_spell(level, ch, victim, DAM_FEAR))
        power /= 2;

    AFFECT_DATA af = {0};
    af.where    = TO_AFFECTS;
    af.level    = level;
    af.point    = &gsn_shadeswarm;

    // Apply up to 3 effects
    int maxEffects(number_range(1, 3));
    for (int i(0); i < maxEffects; ++i)
    {
        switch (number_range(0, 1))
        {
            case 0: // Vertigo
                if (!is_affected(victim, gsn_vertigo) && power >= 300)
                {
                    power -= 300;
                    int maxDuration(UMIN(20, power / 10));
                    af.type = gsn_vertigo;
                    af.duration = number_range(0, maxDuration);
                    affect_to_char(victim, &af);
                    power -= (af.duration * 10);
                }
                break;

            case 1: // Confusion
                if (!is_affected(victim, gsn_confusion) && power >= 150)
                {
                    power -= 150;
                    int maxDuration(UMIN(20, power / 10));
                    af.type = gsn_confusion;
                    af.duration = number_range(0, maxDuration);
                    affect_to_char(victim, &af);
                    power -= (af.duration * 10);
                }
                break;

            case 2: // Manabarbs
                if (!is_affected(victim, gsn_manabarbs) && power >= 150)
                {
                    power -= 150;
                    int maxDuration(UMIN(20, power / 10));
                    af.type = gsn_manabarbs;
                    af.duration = number_range(0, maxDuration);
                    affect_to_char(victim, &af);
                    power -= (af.duration * 10);
                }
                break;

            case 3: // Spirit block
                if (!is_affected(victim, gsn_spiritblock) && power >= 220)
                {
                    power -= 200;
                    int maxModifier(UMIN(4, (power / 20)));
                    af.modifier = -1 * number_range(1, maxModifier);
                    power += (af.modifier * 20);
                    
                    int maxDuration(UMIN(20, power / 10));
                    af.type = gsn_spiritblock;
                    af.duration = number_range(0, maxDuration);
                    af.location = number_range(1, 5);
                    affect_to_char(victim, &af);
                    power -= (af.duration * 10);

                    af.location = 0;
                    af.modifier = 0;
                }
                break;
            
            case 4: // Nightfears
                if (!is_affected(victim, gsn_nightfears) && !IS_OAFFECTED(victim, AFF_NIGHTFEARS) && power >= 250)
                {
                    power -= 250;
                    int maxDuration(UMIN(20, power / 10));
                    af.where = TO_OAFFECTS;
                    af.type = gsn_nightfears;
                    af.duration = number_range(0, maxDuration);
                    af.bitvector = AFF_NIGHTFEARS;
                    af.modifier = (number_range(1, 3) * 100);
                    affect_to_char(victim, &af);

                    af.modifier = 0;
                    af.bitvector = 0;
                    af.where = TO_AFFECTS;
                    power -= (af.duration * 10);
                }
                break;
        }
    }

    // Now apply the actual swarm affect
    int skillLoss(UMIN(19, power / 50));
    power -= skillLoss * 50;

    af.type = sn;
    af.duration = number_range(5, 15);
    af.modifier = 1 + skillLoss;
    affect_to_char(victim, &af);

    if (power > 50 && victim->mana > 0)
    {
        // Drain mana if there is enough power left
        send_to_char("You feel your energy sapped by the spectres within you!\n", victim);
        victim->mana = UMAX(0, victim->mana - power);
    }

    return true;
}

bool spell_fugue(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    // Check for cooldown
    AFFECT_DATA * paf(get_affect(ch, sn));
    if (paf != NULL)
    {
        if (paf->location != APPLY_NONE) send_to_char("You are already moving about as if in a dream.\n", ch);
        else send_to_char("If you spend all your time in a fugue you will lose sight of reality.\n", ch);
        return false;
    }

    // Lookup the incense
    OBJ_DATA * obj(lookup_obj_extra_flag(ch, ITEM_INCENSE));
    if (obj == NULL)
    {
        send_to_char("You cannot enter a fugue state without incense to relax your mind.\n", ch);
        return false;
    }

    // Check position
    if (ch->position != POS_RESTING)
    {
        send_to_char("You must be in a relaxed position to enter a fugue state.\n", ch);
        return false;
    }

    struct CallbackHandler
    {
        static bool HandleMoveFrom(CHAR_DATA * ch, ROOM_INDEX_DATA *, EchoAffect *, void *)
        {
            send_to_char("You stir, disturbing your meditation before your mind has fully entered a fugue state.\n", ch);
            return true;
        }

        static bool HandlePositionChange(CHAR_DATA * ch, int newPos, EchoAffect *, void *)
        {
            if (newPos == POS_RESTING)
                return false;

            send_to_char("You stir, disturbing your meditation before your mind has fully entered a fugue state.\n", ch);
            return true;
        }

        static bool FinishSpell(CHAR_DATA * ch, EchoAffect *, void * tag)
        {
            // Apply the effect
            int level(reinterpret_cast<int>(tag));
            AFFECT_DATA af = {0};
            af.where    = TO_AFFECTS;
            af.type     = gsn_fugue;
            af.level    = level;
            af.duration = (level / 6) + number_range(0, 4);
            af.location = APPLY_RESIST_ILLUSION;
            af.modifier = -40;
            affect_to_char(ch, &af);
            return false;
        }
    };

    // Echo the start of the effect
    act("You light $p and settle into a comfortable position, ready to relax your mind and spirit.", ch, obj, NULL, TO_CHAR);
    act("$n lights $p and settles into a comfortable position.", ch, obj, NULL, TO_ROOM);
    extract_obj(obj);

    // Set up the rest of the echoes
    EchoAffect * echoAff(new EchoAffect(1));
    echoAff->SetTag(reinterpret_cast<void*>(level));
    echoAff->SetMoveCallback(&CallbackHandler::HandleMoveFrom);
    echoAff->SetPositionCallback(&CallbackHandler::HandlePositionChange);
    echoAff->AddLine(NULL,
                    "Smoke wafts up about you, setting your soul at ease as your breathing becomes slow and deep.",
                    "Smoke wafts up about $n, and $e begins to breathe slowly and deeply.");

    echoAff->AddLine(NULL,
                    "As you fall into a deep trance, the world around you grows hazy and dreamlike.",
                    "$n's gaze grows unfocused as wisps of fragrant smoke continue to curl around $m.");

    echoAff->AddLine(&CallbackHandler::FinishSpell,
                    "You complete your meditations as the last of the incense burns away, leaving you fully entranced.",
                    "The last of $n's incense burns away, leaving $s face blank and tranquil.");

    EchoAffect::ApplyToChar(ch, echoAff);
    return true;
}

bool spell_etherealbrethren(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    // Sanity-check
    if (ch->in_room == NULL)
    {
        bug("etherealbrethren called from null room", 0);
        return false;
    }

    // Check for cooldown; if less than 60 seconds since last casting, the power will be weaker
    AFFECT_DATA * paf(get_affect(ch, sn));
    bool recentCasting(paf != NULL && (current_time - paf->modifier) <= 60);

    // Find some shades
    std::vector<CHAR_DATA*> shades;
    bool anyFound(false);
    int caster_grade(aura_grade(ch));
    for (CHAR_DATA * shade(ch->in_room->people); shade != NULL; shade = shade->next_in_room)
    {
        // Filter out the shade
        if (!IS_NPC(shade) || !IS_SET(shade->nact, ACT_SHADE))
            continue;

        anyFound = true;
        int resistance(20 - get_curr_stat(ch, STAT_CHR) + (recentCasting ? 20 : 0));
        if (IS_GOOD(shade))
        {
            if (caster_grade < 0)
            {
                // Good shades will help goodies, no questions asked
                shades.push_back(shade);
                continue;
            }

            // Non-goody casters have to convince them; five levels for every grade of evil
            resistance = caster_grade * 5;
        }
        else if (IS_EVIL(shade))
        {
            // Evils always have to be convinced, but especially so if the caster is a goody
            if (caster_grade < 0)
                resistance = caster_grade * -5;
        }
        
        // Check for saves
        if (!saves_spell(level - resistance, ch, shade, DAM_CHARM))
            shades.push_back(shade);
    }

    // If there were no shades, echo appropriately
    if (!anyFound)
    {
        send_to_char("There are no shades here to invite in.\n", ch);
        return false;
    }

    if (recentCasting)
    {
        act("You hold out a hand in invitation, and weak strands of pale light flow from it, barely penetrating into the spirit realm.", ch, NULL, NULL, TO_CHAR);
        act("$n extends a hand, and pale strands of silver light flow from it, trailing off into nothingness.", ch, NULL, NULL, TO_ROOM);
    }
    else
    {
        act("You hold out a hand in invitation, letting soft tendrils of silver light flow from it into the spirit realm.", ch, NULL, NULL, TO_CHAR);
        act("$n extends a hand, and soft tendrils of silver light flow from it before fading into obscurity.", ch, NULL, NULL, TO_ROOM);
    }

    // There were shades; did any of them want to come in?
    if (shades.empty())
    {
        send_to_char("Nothing from the spirit realm responds to your invitation.\n", ch);
        return true;
    }

    // Send in the shades
    int power(0);
    for (size_t i(0); i < shades.size(); ++i)
    {
        act("$N follows a tendril of silver light back to you, accepting the invitation.", ch, NULL, shades[i], TO_CHAR);

        std::ostringstream mess;
        mess << shades[i]->short_descr << " follows a tendril of silver light back to $N, vanishing as it reaches $M.";
        for (CHAR_DATA * echoChar(ch->in_room->people); echoChar != NULL; echoChar = echoChar->next_in_room)
        {
            if (can_see(echoChar, shades[i]) && echoChar != ch)
                act(mess.str().c_str(), echoChar, NULL, ch, TO_CHAR);
        }

        power += (shades[i]->level * shades[i]->timer);
        extract_char(shades[i], true);
    }

    send_to_char("You feel invigorated you as you draw on the expertise and memories of your newfound brethren!\n", ch);

    power /= 180;
    if (paf == NULL)
    {
        // Add the effect
        AFFECT_DATA af = {0};
        af.where    = TO_AFFECTS;
        af.type     = sn;
        af.level    = level;
        af.modifier = current_time;
        af.duration = power;
        affect_to_char(ch, &af);
    }
    else
    {
        // Update the effct
        paf->level    = level;
        paf->duration += power;
        paf->modifier = current_time;
    }

    return true;
}

bool spell_nourishspirit(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    CHAR_DATA * victim(static_cast<CHAR_DATA*>(vo));

    // Check for already present
    if (is_affected(victim, sn))
    {
        if (victim == ch) send_to_char("Your spirit is already well-nourished.\n", ch);
        else act("$N's spirit is already well-nourished.", ch, NULL, victim, TO_CHAR);
        return false;
    }

    // Apply the affect
    AFFECT_DATA af = {0};
    af.where    = TO_AFFECTS;
    af.type     = sn;
    af.level    = level;
    af.duration = 6 + level;
    af.location = APPLY_MANA;
    af.modifier = level * 2;
    affect_to_char(victim, &af);
    victim->mana += af.modifier;

    af.location = APPLY_SAVING_SPELL;
    af.modifier = -level / 8;
    affect_to_char(victim, &af);

    // Echo about it
    send_to_char("Your spirit seems stronger, its fears soothed and doubts quelled.\n", victim);
    if (ch != victim)
        act("You strengthen $N's spirit, soothing its fears and quelling $S doubts.", ch, NULL, victim, TO_CHAR);

    return true;
}

void strip_avatar(CHAR_DATA * ch)
{
    affect_strip(ch, gsn_avatar);
    affect_strip(ch, gsn_avatarofthelodestar);
    affect_strip(ch, gsn_avataroftheannointed);
    affect_strip(ch, gsn_avataroftheprotector);
}

int type_of_avatar(CHAR_DATA * ch)
{
    // Check for cooldown mode from avatar, which implies not currently an avatar
    if (IS_NAFFECTED(ch, AFF_AVATAR))
        return -1;

    // Check for one of the avatar effects
    for (AFFECT_DATA * paf(ch->affected); paf != NULL; paf = paf->next)
    {
        if (paf->type == gsn_avatar || paf->type == gsn_avatarofthelodestar
        ||  paf->type == gsn_avataroftheprotector || paf->type == gsn_avataroftheannointed)
            return paf->type;
    }

    return -1;
}

bool is_an_avatar(CHAR_DATA * ch) {return (type_of_avatar(ch) != -1);}

static bool do_avatar_common(CHAR_DATA * ch, const char * spellName, int sn, int level, int hitdamBonus, int hpBonus)
{
    // Check for cooldown
    if (IS_NAFFECTED(ch, AFF_AVATAR))
    {
        send_to_char("You can't assume the form of an avatar again so soon.\n", ch);
        return false;
    }

    // Get some quintessence
    OBJ_DATA * quintessence(require_quintessence(ch));
    if (quintessence == NULL)
        return false;


    // Echo the spell
    act("$n rises from the ground, trembling, as $s body explodes in brilliant light!", ch, NULL, NULL, TO_ROOM);
    act("You rise from the ground, trembling uncontrollably.", ch, NULL, NULL, TO_CHAR);
    
    std::ostringstream mess;
    mess << "Your body explodes in pain and power as you transform into an avatar of the " << spellName << "!\n";
    send_to_char(mess.str().c_str(), ch);

    // Change up the long desc
    if (ch->long_descr && ch->long_descr != &str_empty[0])
        free_string(ch->long_descr);

    ch->long_descr = str_dup("A shifting being of pure spirit floats through the area, bathing it in an eerie blue light.\n");
    affect_strip(ch, gsn_sanctuary);

    // Apply avatar effect
    AFFECT_DATA af = {0};
    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = -1;
    af.location  = APPLY_HIT;
    af.modifier  = dice(level, 21) + hpBonus;
    affect_to_char(ch, &af);

    af.location  = APPLY_HITROLL;
    af.modifier  = UMAX(1, level + hitdamBonus);
    affect_to_char(ch, &af);

    af.location  = APPLY_DAMROLL;
    af.modifier  = UMAX(1, level + hitdamBonus);
    affect_to_char(ch, &af);
    ch->hit = ch->max_hit;

    // Consume the quintessence and strip weavesense
    affect_strip(ch, gsn_weavesense);
    consume_quintessence(ch, quintessence);
    return true;
}

bool spell_avatarofthelodestar(int sn, int level, CHAR_DATA * ch, void * vo, int target) {return do_avatar_common(ch, "Lodestar", sn, level, 0, 0);}
bool spell_avataroftheannointed(int sn, int level, CHAR_DATA * ch, void * vo, int target) {return do_avatar_common(ch, "Annointed", sn, level, 0, 0);}

bool spell_avataroftheprotector(int sn, int level, CHAR_DATA * ch, void * vo, int target) 
{
    if (!do_avatar_common(ch, "Protector", sn, level, -10, 100))
        return false;

    // Redirect all fighting against the group to the avatar
    if (ch->in_room == NULL)
        return true;
    
    bool found(false);
    for (CHAR_DATA * victim(ch->in_room->people); victim != NULL; victim = victim->next_in_room)
    {
        // Filter out anyone not fighting a groupmate
        if (victim->fighting == NULL || !is_same_group(ch, victim->fighting) || victim->fighting == ch)
            continue;

        // Redirect to the caster
        stop_fighting(victim);
        check_killer(ch, victim);
        set_fighting(victim, ch);
        found = true;
    }

    if (found)
    {
        send_to_char("You leap to the fore, shielding your group from harm!\n", ch);
        act("$n leaps to the fore, shielding $s group from harm!", ch, NULL, NULL, TO_ROOM);
    }

    return true;
}

bool spell_attunefount(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    // Check for a fount here
    if (ch->in_room == NULL || !Weave::HasWeave(*ch->in_room) || !ch->in_room->ley_group->HasFount())
    {
        send_to_char("There is no fount here to attune.\n", ch);
        return false;
    }

    // Check whether already attuned to this caster
    if (ch->in_room->ley_group->AttunedID() == ch->id)
    {
        send_to_char("This fount is already attuned to your spirit.\n", ch);
        return false;
    }

    // Set up the attunement process
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

        static bool FinishSpell(CHAR_DATA * ch, EchoAffect *, void *) 
        {
            ch->in_room->ley_group->SetAttunedID(ch->id);
            Weave::UpdateAttunements();
            Weave::SaveWeave();
            return false;
        }

        private:
            static void HandleCancel(CHAR_DATA * ch) {send_to_char("You abandon your ritual, and the fount's energy return to normal.\n", ch);}
    };

    // Handle echoes
    act("You begin a low chant, holding your arms out to the sides with your palms facing downwards.", ch, NULL, NULL, TO_CHAR);
    act("$n begins a low chant, holding $s arms out to the sides with $s palms facing downwards.", ch, NULL, NULL, TO_ROOM);

    EchoAffect * echoAff(new EchoAffect(1));
    echoAff->SetMoveCallback(&CallbackHandler::HandleMoveFrom);
    echoAff->SetPositionCallback(&CallbackHandler::HandlePositionChange);
    echoAff->AddLine(NULL,
                    "You pitch your chant higher, bringing your arms in front of you and turning your palms inwards.",
                    "$n pitches $s chant higher, bringing $s arms in front of $mself and turning $s palms inwards.");

    echoAff->AddLine(NULL,
                    "Visible energy crackles between your hands as they meet! You sense the power of the fount flicker and warp!",
                    "Visible energy crackles between $n's hands as they meet, still stretched out in front of $m.");

    echoAff->AddLine(&CallbackHandler::FinishSpell,
                    "You slam your hands into your own chest to complete the rite, feeling the fount's energy surge just behind them and into you!",
                    "$n suddenly slams $s hands into $s own chest, and sparks of energy seem to fly off $m for a brief moment before gradually dissipating.");

    EchoAffect::ApplyToChar(ch, echoAff);
    return true;
}

bool spell_singularity(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    // Sanity-check
    if (ch->in_room == NULL)
    {
        bug("Singularity called from null room", 0);
        return false;
    }

    // Verify that the caster is on the Weave
    if (!Weave::HasWeave(*ch->in_room))
    {
        send_to_char("The Weave is not strong enough here to see all points as one.\n", ch);
        return false;
    }

    std::ostringstream mess;
    send_to_char("You concentrate on the Weave around you, sending your thoughts along the threads of quintessence.\n", ch);

    // Iterate the pcs
    for (DESCRIPTOR_DATA * d = descriptor_list; d->next != NULL; d = d->next)
    {
        // Filter out certain characters
        if (d->connected != CON_PLAYING || d->character->in_room == NULL 
        || !Weave::HasWeave(*d->character->in_room) || !can_see(ch, d->character)
        || d->character == ch || is_affected(d->character, gsn_shadow_ward))
            continue;

        // Circle of protection can shield a target
        if (is_symbol_present(*d->character->in_room, OBJ_VNUM_SYMBOL_PROTECT))
            continue;

        // Show the character's location to the caster
        mess << d->character->name << " is in " << d->character->in_room->name << "\n";
    }

    // Echo the locations
    if (mess.str().empty())
        send_to_char("After a time, however, you give up, having found nothing of interest.\n", ch);
    else
    {
        send_to_char("Through the whorls and patterns of energy, you sense...\n", ch);
        send_to_char(mess.str().c_str(), ch);
    }

    return true;
}

bool spell_leypulse(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    // Sanity-check
    if (ch->in_room == NULL)
    {
        bug("Ley pulse called from null room", 0);
        return false;
    }
    
    // Verify that the caster is on the Weave
    if (!Weave::HasWeave(*ch->in_room))
    {
        send_to_char("The Weave is not strong enough here to send a pulse through it.\n", ch);
        return false;
    }

    // Check for a target
    if (target_name[0] == '\0')
    {
        send_to_char("At whom did you wish to send the pulse?\n", ch);
        return false;
    }

    // Get the target
    CHAR_DATA * victim(get_char_world(ch, target_name));
    if (victim == NULL || victim->in_room == NULL || !Weave::HasWeave(*victim->in_room))
    {
        send_to_char("You cannot sense such a person within the Weave.\n", ch);
        return false;
    }

    if (IS_NPC(victim))
    {
        send_to_char("Their presence weighs too lightly on the Weave to target them.\n", ch);
        return false;
    }

    if (victim->in_room == ch->in_room)
    {
        act("But $N is right here!", ch, NULL, victim, TO_CHAR);
        return false;
    }

    // Check for a circle of protection
    if (is_symbol_present(*victim->in_room, OBJ_VNUM_SYMBOL_PROTECT))
    {
        act("$N is shielded from your magics.", ch, NULL, victim, TO_CHAR);
        return false;
    }

    // Effect of the pulse differs by target's karma
    int mod(aura_grade(victim));
    if (mod <= 0)
    {
        // Neutral or good-aligned; restore their mana (capped at the cost of this spell)
        act("A steady flow of golden energy suddenly appears, streaming into $n from all about.", victim, NULL, NULL, TO_ROOM);
        if (victim->mana >= victim->max_mana)
        {
            send_to_char("A pleasant flow of energy streams into you from all about, though with little effect.\n", victim);
            act("You send a pulse of energy through the Weave towards $N, but it has little effect on $M.", ch, NULL, victim, TO_CHAR);
            return true;
        }

        send_to_char("A pleasant flow of energy streams into you from all about, restoring your mental energy!\n", victim);
        act("You send a pulse of energy through the Weave towards $N, refreshing $S mind!", ch, NULL, victim, TO_CHAR);
        victim->mana = UMIN(victim->max_mana, victim->mana + UMIN(skill_table[sn].min_mana, (100 + (abs(mod) * 100))));
        return true;
    }
    
    // Evil; check for safe
    if (is_safe_spell(ch, victim, false))
    {
        act("You send a pulse of energy through the Weave to strike $N, but cannot harm $M.\n", ch, NULL, victim, TO_CHAR);
        return true;
    }
    
    // Check for a save
    act("A powerful surge of golden energy suddenly strikes $n!", victim, NULL, NULL, TO_ROOM);
    if (saves_spell(level, ch, victim, DAM_ENERGY))
    {
        send_to_char("A powerful pulse of energy strikes you from all about, but you manage to shake it off.\n", victim);
        act("You send a pulse of energy through the Weave to strike $N, but $E shakes it off.", ch, NULL, victim, TO_CHAR);
        return true;
    }

    // Choose a malediction
    switch (number_range(0, 2))
    {
        // Mana damage
        case 0:
            if (victim->mana > 0)
            {
                send_to_char("A powerful pulse of energy strikes you from all about, draining your mana!\n", victim);
                act("You send a pulse of energy through the Weave to strike $N, draining $S mana!", ch, NULL, victim, TO_CHAR);
                int manaDamage(dice(level, 15));
                victim->mana = UMAX(0, victim->mana - manaDamage);
            }
            else
            {
                send_to_char("A pulse of energy strikes you suddenly, but with little effect.\n", victim);
                act("You send a pulse of energy through the Weave to strike $N, but with little effect.", ch, NULL, victim, TO_CHAR);
            }
            break;

        // HP damage
        case 1:
            send_to_char("A powerful pulse of energy strikes you suddenly!\n", victim);
            act("You send a pulse of energy through the Weave, striking $N bodily!", ch, NULL, victim, TO_CHAR);
            sourcelessDamage(victim, "the energy surge", dice(4, level), sn, DAM_ENERGY);
            break;

        // Max-mana damage
        default:
        {
            // Also lag the victim slightly, but mostly for flavor
            send_to_char("A powerful pulse of energy strikes you, leaving you slightly dazed.\n", victim);
            act("You send a pulse of energy through the Weave, dazing $N!", ch, NULL, victim, TO_CHAR);
            WAIT_STATE(victim, UMAX(victim->wait, PULSE_VIOLENCE / 2));
            
            AFFECT_DATA af = {0};
            af.where    = TO_AFFECTS;
            af.type     = sn;
            af.duration = number_range(level / 10, level / 5);
            af.modifier = -level;
            af.location = APPLY_MANA;
            affect_to_char(victim, &af);

            victim->mana = UMIN(victim->mana, victim->max_mana);
            break;
        }
    }

    return true;
}

AFFECT_DATA * get_quintessence_rushing_affect(CHAR_DATA * ch)
{
    for (AFFECT_DATA * paf(get_affect(ch, gsn_quintessencerush)); paf != NULL; paf = get_affect(ch, gsn_quintessencerush, paf))
    {
        if (paf->modifier >= 0)
            return paf;
    }

    return NULL;
}

bool is_quintessence_rushing(CHAR_DATA * ch)
{
    return (get_quintessence_rushing_affect(ch) != NULL);
}

bool spell_quintessencerush(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    // Check for cooldown
    if (is_affected(ch, sn))
    {
        send_to_char("Though your mind might be willing, your body could not handle another rush of quintessence so soon.\n", ch);
        return false;
    }

    // Get some quintessence
    OBJ_DATA * quintessence(require_quintessence(ch));
    if (quintessence == NULL)
        return false;

    // Apply the rush
    AFFECT_DATA af = {0};
    af.where    = TO_AFFECTS;
    af.type     = sn;
    af.level    = level;
    af.location = APPLY_HIT;
    af.modifier = (level * 2);
    af.duration = level / 6;
    affect_to_char(ch, &af);
    ch->hit += af.modifier;

    // Apply the cooldown
    af.location = APPLY_NONE;
    af.modifier = -1;
    af.duration = 22 - UMAX(0, (get_skill(ch, sn) - 60) / 10);
    affect_to_char(ch, &af);

    send_to_char("You enfold your spirit in raw quintessence, and feel your mind expand!\n", ch);
    act("$n's body seems to shimmer briefly with a soft golden glow.", ch, NULL, NULL, TO_ROOM);

    // Consume the quintessence    
    consume_quintessence(ch, quintessence);
    return true;
}

bool spell_manifestweave(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    // Sanity-check
    if (ch->in_room == NULL)
    {
        bug("manifest weave called from null room", 0);
        return false;
    }

    // Check for existing weave walls
    for (unsigned int i(0); i < Direction::Max; ++i)
    {
        if (ch->in_room->exit[i] != NULL && IS_SET(ch->in_room->exit[i]->exit_info, EX_WEAVEWALL))
        {
            send_to_char("The Weave is already manifested here.\n", ch);
            return false;
        }
    }

    // Check for ley lines
    if (!Weave::HasWeave(*ch->in_room))
    {
        send_to_char("There is no Weave here to manifest.\n", ch);
        return false;
    }

    // Figure out the direction
    if (target_name[0] == '\0')
    {
        send_to_char("In which direction did you wish to manifest the Weave?\n", ch);
        return false;
    }

    Direction::Value direction(Direction::ValueFor(target_name));
    if (direction == Direction::Max)
    {
        send_to_char("That's not a valid direction.\n", ch);
        return false;
    }

    // Check the room via the specified direction
    ROOM_INDEX_DATA * room(Direction::Adjacent(*ch->in_room, direction));
    if (room == NULL)
    {
        send_to_char("That way is already blocked.\n", ch);
        return false;
    }

    // Block the specified direction
    SET_BIT(ch->in_room->exit[direction]->exit_info, EX_WEAVEWALL);
    std::ostringstream mess;
    mess << "A golden mesh of energy and light shimmers into being " << Direction::DirectionalNameFor(direction) << ", sealing off the way.";
    act(mess.str().c_str(), ch, NULL, NULL, TO_ALL);

    AFFECT_DATA af = {0};
    af.where    = TO_ROOM_AFF;
    af.type     = sn;
    af.level    = level;
    af.duration = number_range(1, 4);
    af.modifier = direction;
    affect_to_room(ch->in_room, &af);

    // If it exists and references back to this room, block the adjacent room as well
    Direction::Value reverse(Direction::ReverseOf(direction));
    if (room->exit[reverse] != NULL && room->exit[reverse]->u1.to_room == ch->in_room)
    {
        SET_BIT(room->exit[reverse]->exit_info, EX_WEAVEWALL);
        af.modifier = reverse;
        affect_to_room(room, &af);

        if (room->people != NULL)
        {
            std::ostringstream mess;
            mess << "A golden mesh of energy and light shimmers into being " << Direction::DirectionalNameFor(reverse) << ", sealing off the way.";
            act(mess.str().c_str(), room->people, NULL, NULL, TO_ALL);
        }
    }

    return true;
}

bool spell_sunderweave(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    // Sanity check
    if (ch->in_room == NULL)
    {
        bug("sunder weave called from null room", 0);
        return false;
    }

    // Check for cooldown
    if (is_affected(ch, sn))
    {
        send_to_char("You are not yet prepared to sunder the Weave again.\n", ch);
        return false;
    }

    // Check for area effect already present
    if (area_is_affected(ch->in_room->area, sn))
    {
        send_to_char("The Weave is already sundered here.\n", ch);
        return false;
    }

    // Apply the effect
    AFFECT_DATA af = {0};
    af.where    = TO_AREA;
    af.type     = sn;
    af.level    = level;
    af.duration = 8;
    affect_to_area(ch->in_room->area, &af);

    // Apply cooldown
    af.where    = TO_AFFECTS;
    af.duration = 120;
    affect_to_char(ch, &af);

    act("You throw your arms back and howl out a single word of arcane power!", ch, NULL, NULL, TO_CHAR);
    act("$n throws $s arms back and howls out a single word of arcane power!", ch, NULL, NULL, TO_ROOM);

    // Echo to PCs in the area
    for (DESCRIPTOR_DATA * d(descriptor_list); d != NULL; d = d->next)
    {
        // Filter out PCs not in the same area
        if (d->character == NULL || d->character->in_room == NULL || d->character->in_room->area != ch->in_room->area)
            continue;

        // Only inform spellcasters
        if (is_spellcaster(d->character))
            send_to_char("You feel your connection to magic fray as the Weave tears asunder!\n", d->character);
    }

    return true;
}

bool spell_etherealsplendor(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    CHAR_DATA * victim(static_cast<CHAR_DATA*>(vo));

    // Check for spell already present
    if (is_affected(victim, sn))
    {
        if (victim == ch) send_to_char("You are already resplendent with ethereal glory.\n", ch);
        else act("$N is already resplendent with ethereal glory.", ch, NULL, victim, TO_CHAR);
        return false;
    }

    // Apply the effect
    AFFECT_DATA af = {0};
    af.where    = TO_AFFECTS;
    af.type     = sn;
    af.level    = level;
    af.duration = level;
    af.location = APPLY_CHR;
    af.modifier = 1 + (level / 17);
    affect_to_char(victim, &af);

    act("The air around you shimmers as you begin to shine with ethereal splendor!", victim, NULL, NULL, TO_CHAR);
    act("The air around $n shimmers as $e begins to shine with ethereal splendor.", victim, NULL, NULL, TO_ROOM);
    return true;
}

bool spell_rebukeofinvesi(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    // Sanity-check
    if (ch->in_room == NULL)
    {
        bug("Rebuke of in'vesi called from null room", 0);
        return false;
    }

    // Check for cooldown
    if (is_affected(ch, sn))
    {
        send_to_char("You are not ready to rebuke evildoers again just yet.\n", ch);
        return false;
    }

    // Apply cooldown
    AFFECT_DATA af = {0};
    af.where    = TO_AFFECTS;
    af.type     = sn;
    af.level    = level;
    af.duration = 12;
    affect_to_char(ch, &af);

    struct CallbackHandler
    {
        static bool HandleMoveFrom(CHAR_DATA * ch, ROOM_INDEX_DATA * room, EchoAffect * thisAffect, void * tag)
        {
            send_to_char("You leave behind your holy rebuke, and its power slowly fades away.\n", ch);
            return true;
        }

        static bool FinishSpell(CHAR_DATA * ch, EchoAffect *, void * tag)
        {
            int level(reinterpret_cast<int>(tag));

            // Damage all evil aura folk in the room
            CHAR_DATA * next_victim;
            for (CHAR_DATA * victim(ch->in_room->people); victim != NULL; victim = next_victim)
            {
                next_victim = victim->next_in_room;

                // If evil, take a lot of saveless light damage
                if (aura_grade(victim) > 0)
                    damage_old(ch, victim, dice(level, 13), gsn_rebukeofinvesi, DAM_LIGHT, true);
                else
                    send_to_char("Several shards of light strike you, but glance harmlessly away.\n", victim);
            }

            return false;
        }
    };
 
    act("You throw your head back and shout a holy word to the skies!", ch, NULL, NULL, TO_CHAR);
    act("$n throws $s head back and shouts a holy word to the skies!", ch, NULL, NULL, TO_ROOM);
    act("In response, a bright white light suddenly appears, coalescing rapidly into a glassy, glowing orb.", ch, NULL, NULL, TO_ALL);

    // Prepare the echo affect
    EchoAffect * echoAff(new EchoAffect(1));
    echoAff->SetTag(reinterpret_cast<void*>(level));
    echoAff->SetMoveCallback(&CallbackHandler::HandleMoveFrom);
    echoAff->AddLine(&CallbackHandler::FinishSpell,
                    "The orb suddenly shatters, scattering jagged bits of glassy luminescence in all directions!",
                    "The orb suddenly shatters, scattering jagged bits of glassy luminescence in all directions!");
    EchoAffect::ApplyToChar(ch, echoAff);
    return true;
}

bool spell_wardoftheshieldbearer(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    // Check for already existing
    AFFECT_DATA * paf(get_affect(ch, sn));
    if (paf == NULL)
    {
        // Apply effect
        AFFECT_DATA af = {0};
        af.where    = TO_AFFECTS;
        af.type     = sn;
        af.level    = level;
        af.duration = -1;
        affect_to_char(ch, &af);

        act("A nimbus of silvery light dances about you as you prepare to ward your group.", ch, NULL, NULL, TO_CHAR);
        act("$n utters an arcane word, and a nimbus of silvery light dances about $m.", ch, NULL, NULL, TO_ROOM);
        return true;
    }

    // Effect already present, remove it
    affect_remove(ch, paf);
    act("The nimbus of silvery light about you fades away as you drop your ward.", ch, NULL, NULL, TO_CHAR);
    act("The nimbus of silvery light about $n fades away.", ch, NULL, NULL, TO_ROOM);
    return true;
}

bool spell_callhost(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    // Sanity check
    if (ch->in_room == NULL)
    {
        bug("Call host called from null room", 0);
        return false;
    }

    // Check for cooldown
    if (is_affected(ch, sn))
    {
        send_to_char("If you call upon the heavenly host too often there will be none left to fight the rest of the forces of the darkness!\n", ch);
        return false;
    }

    // Get some quintessence
    OBJ_DATA * quintessence(require_quintessence(ch));
    if (quintessence == NULL)
        return false;

    // Call the host down; three to five seraphim
    int count(number_range(3, 5));
    for (int i(0); i < count; ++i)
    {
        // Create the angel
        CHAR_DATA * angel(create_mobile(get_mob_index(MOB_VNUM_BATTLESERAPH)));
        angel->level     = UMAX(1, level - 2);
        angel->max_hit   = dice(20, level) + 400;
        angel->hit       = angel->max_hit;
        angel->damroll   = 5;
        angel->hitroll   = 5 + (level / 2);
        angel->damage[0] = 7;
        angel->damage[1] = 3 + (level / 5);
        angel->damage[2] = 5;
        angel->armor[0]  = 0 - ((level * 15) / 10);
        angel->armor[1]  = 0 - ((level * 15) / 10);
        angel->armor[2]  = 0 - ((level * 15) / 10);
        angel->armor[3]  = 0 - ((level * 15) / 10);

        // Focus the angel on the caster
        angel->memfocus[0] = ch;
        char_to_room(angel, ch->in_room);
    }

    act("You infuse your voice with quintessence to call to the heavens, and are answered by the swift arrival of several seraphim!", ch, NULL, NULL, TO_CHAR);
    act("$n raises his voice to the heavens, and is answered by the swift arrival of several seraphim!", ch, NULL, NULL, TO_ROOM);

    // Apply a cooldown
    AFFECT_DATA af = {0};
    af.where    = TO_AFFECTS;
    af.type     = sn;
    af.level    = level;
    af.duration = 30;
    affect_to_char(ch, &af);

    // Consume the quintessence    
    consume_quintessence(ch, quintessence);
    return true;
}

bool spell_bindessence(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    // Get some quintessence
    std::vector<OBJ_DATA *> quintessence(require_quintessence(ch, 2));
    if (quintessence.empty())
        return false;

    // Get the object and check for special cases
    OBJ_DATA * obj(static_cast<OBJ_DATA*>(vo));
    int affectCount(count_obj_affects(obj, sn));
    
    // Prepare an affect
    AFFECT_DATA af = {0};
    af.type     = sn;
    af.where    = TO_OBJECT;
    af.level    = level;
    af.duration = -1;

    switch (obj->pIndexData->vnum)
    {
        case OBJ_VNUM_CELESTIALBULWARK:
        {
            // Add to the magic AC value and one other, chosen at random
            ++obj->value[3];
            ++obj->value[number_range(0, 2)];

            // Determine other bonuses
            bool ignore(false);
            int num(number_range(0, 100 + 5 * affectCount * affectCount));
            if (num < 10) {af.location = APPLY_RESIST_NEGATIVE; af.modifier = 1;}
            else if (num < 20) {af.location = APPLY_RESIST_ENERGY; af.modifier = 1;}
            else if (num < 50) {af.location = APPLY_HIT; af.modifier = number_range(1, 3);}
            else ignore = true;

            if (!ignore)
                affect_to_obj(obj, &af);
            
            act("You bind energy into $p, feeling its power surge as it grows ever more ready to ward against the ravages of darkness.", ch, obj, NULL, TO_CHAR);
            act("A brilliant white light shines forth from $p as $n chants over it!", ch, obj, NULL, TO_ROOM);
            consume_quintessence(ch, quintessence);
            return true;
        }

        case OBJ_VNUM_ETHEREALSKEAN:
        {
            // Add to the damage, hitroll, and damroll according to the schedule
            switch (affectCount)
            {
                case 0: ++obj->value[1]; ++obj->value[2]; break;
                case 1: obj->value[2] += 2; af.location = APPLY_HITROLL; af.modifier = 1; break;
                case 2: ++obj->value[2]; af.location = APPLY_DAMROLL; af.modifier = 1; break;
                case 3: ++obj->value[1]; af.location = APPLY_HITROLL; af.modifier = 1; break;
                case 4: obj->value[2] += 2; af.location = APPLY_DAMROLL; af.modifier = 1; break;
                case 5: ++obj->value[2]; break;
                case 6: af.bitvector = ITEM_BLESS; af.location = (number_bits(1) == 0 ? APPLY_DAMROLL : APPLY_HITROLL); af.modifier = 1; break; 
                case 7: 
                case 9: obj->value[2] += 2; af.location = APPLY_DAMROLL; af.modifier = number_range(0, 1); break;
                case 8: 
                case 10: obj->value[2] += 2; af.location = APPLY_HITROLL; af.modifier = number_range(0, 1); break;
                default:
                    if (number_bits(5) == 0)
                    {
                        af.location = (number_bits(0) == 0 ? APPLY_HITROLL : APPLY_DAMROLL);
                        af.modifier = 1;
                    }
                    break;
            }
            affect_to_obj(obj, &af);
            
            // Repair the skean
            obj->condition = 100;
            act("$p glows with a muted golden light as you infuse it with more quintessence.", ch, obj, NULL, TO_CHAR);
            act("$p glows with a muted golden light.", ch, obj, NULL, TO_ROOM);
            consume_quintessence(ch, quintessence);
            return true;
        }

        case OBJ_VNUM_RODOFTRANSCENDENCE:
        {
            // Add to the damage according to a schedule
            switch (affectCount)
            {
                case 0: 
                case 2:
                case 4: ++obj->value[2]; break;
                case 1:
                case 3:
                case 5:
                case 6:
                case 7:
                case 8:
                case 9: ++obj->value[1]; break;
            }

            // Check for flags
            switch (number_range(0, 20))
            {
                case 0: af.bitvector = ITEM_NODESTROY; break;
                case 1: af.bitvector = ITEM_BLESS; break;
                case 2: af.bitvector = ITEM_NODISARM; break;
            }

            affect_to_obj(obj, &af);
            act("$p shines a pure white, growing in power as you bind energy within.", ch, obj, NULL, TO_CHAR);
            act("$p shines with a pure white light.", ch, obj, NULL, TO_ROOM);
            consume_quintessence(ch, quintessence);
            return true;
        }

        case OBJ_VNUM_CHALICEOFTRUTH:
        case OBJ_VNUM_CHAPLETOFFERVOR:
        case OBJ_VNUM_LENSOFDIVINATION:
            act("$p cannot be bound with any more quintessence.", ch, obj, NULL, TO_CHAR);
            return false;
    }

    // Check for a binding already being present
    if (affectCount > 0)
    {
        act("$p cannot be bound with any more quintessence.", ch, obj, NULL, TO_CHAR);
        return false;
    }

    // Bind effects vary by type and wear loc
    af.bitvector = ITEM_MAGIC;
    if (obj->item_type == ITEM_WEAPON)
    {
        // Weapons which cannot be consecrated cannot be bound
        // Weapon effects handled in fight.c
        if (!check_can_consecrate(ch, obj))
            return false;
    }
    else
    {
        if (CAN_WEAR(obj, ITEM_WEAR_HEAD)) {}        // Head effects handled in magic.c
        else if (CAN_WEAR(obj, ITEM_WEAR_HANDS)) {}  // Hand effects handled in fight.c
        else if (CAN_WEAR(obj, ITEM_WEAR_NECK))  {af.location = APPLY_SAVES; af.modifier = -1 * number_range(1, 1 + (level / 50));}
        else if (CAN_WEAR(obj, ITEM_WEAR_BODY))  {af.location = APPLY_HIT; af.modifier = number_range(level / 4, level / 2);}
        else if (CAN_WEAR(obj, ITEM_WEAR_ABOUT)) {af.location = APPLY_RESIST_NEGATIVE; af.modifier = number_range(level / 20, level / 12);}
        else if (CAN_WEAR(obj, ITEM_WEAR_FEET))  {af.location = APPLY_MOVE; af.modifier = number_range(level / 4, level / 2);}
        else
        {
            // Anything else gets mana
            af.location = APPLY_MANA;
            af.modifier = number_range(level / 12, level / 6);
        }
    }

    // Apply the effect
    af.type = sn;
    affect_to_obj(obj, &af);

    act("As you chant softly, threads of pale golden energy wrap themselves about $p, infusing it with your power.", ch, obj, NULL, TO_CHAR);
    act("$n chants softly, and a pale golden light briefly shines about $p.", ch, obj, NULL, TO_ROOM);

    // Consume the quintessence    
    consume_quintessence(ch, quintessence);
    return true;
}

bool spell_workessence(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    // Make sure an argument was supplied
    if (target_name[0] == '\0')
    {
        send_to_char("What did you wish to weave from quintessence?\n", ch);
        return false;
    }

    // Determine which item to work
    int vnum;
    unsigned int cost;
    if (!str_prefix(target_name, "chalice"))        {vnum = OBJ_VNUM_CHALICEOFTRUTH;        cost = 8;}
    else if (!str_prefix(target_name, "chaplet"))   {vnum = OBJ_VNUM_CHAPLETOFFERVOR;       cost = 7;} 
    else if (!str_prefix(target_name, "bulwark"))   {vnum = OBJ_VNUM_CELESTIALBULWARK;      cost = 4;} 
    else if (!str_prefix(target_name, "skean"))     {vnum = OBJ_VNUM_ETHEREALSKEAN;         cost = 4;} 
    else if (!str_prefix(target_name, "lens"))      {vnum = OBJ_VNUM_LENSOFDIVINATION;      cost = 7;} 
    else if (!str_prefix(target_name, "rod"))       {vnum = OBJ_VNUM_RODOFTRANSCENDENCE;    cost = 4;} 
    else
    {
        send_to_char("You do not know how to work such a thing.\n", ch);
        return false;
    }

    // Verify the quintessence cost
    std::vector<OBJ_DATA *> quintessence(require_quintessence(ch, cost));
    if (cost > 0 && quintessence.empty())
        return false;

    // Make the object
    OBJ_INDEX_DATA * objIndex(get_obj_index(vnum));
    if (objIndex == NULL)
    {
        send_to_char("An error has occurred; please inform the gods.\n", ch);
        bug("Unable to find object index for work essence", 0);
        return false;
    }

    OBJ_DATA * obj(create_object(objIndex, 0));
    if (obj == NULL)
    {
        send_to_char("An error has occurred; please inform the gods.\n", ch);
        bug("Unable to create work essence object", 0);
        return false;
    }

    obj->level = level;
    act("You summon strands of pure energy, weaving them together into $p.", ch, obj, NULL, TO_CHAR);
    act("$n summons strands of pure energy, weaving them together into $p.", ch, obj, NULL, TO_ROOM);
    obj_to_char(obj, ch);

    // Consume the quintessence    
    consume_quintessence(ch, quintessence);
    return true;
}

std::vector<OBJ_DATA*> find_posterns_for(long id)
{
    std::vector<OBJ_DATA*> result;

    // Iterate the object list, looking for posterns
    for (OBJ_DATA * obj(object_list); obj != NULL; obj = obj->next)
    {
        if (obj->pIndexData->vnum != OBJ_VNUM_POSTERN || obj->in_room == NULL)
            continue;

        // Look up the postern affects to see whether it is keyed to the caster
        for (AFFECT_DATA * paf(get_obj_affect(obj, gsn_forgepostern)); paf != NULL; paf = get_obj_affect(obj, gsn_forgepostern, paf))
        {
            if (paf->modifier == id)
            {
                // The postern is keyed to the caster
                result.push_back(obj);
                break;
            }
        }
    }

    return result;
}

void handle_remove_postern(OBJ_DATA * postern, const std::vector<OBJ_DATA*> & posterns)
{
    // Sanity-check
    if (postern->in_room == NULL)
    {
        bug("remove_postern: target postern not in room", 0);
        return;
    }

    // Find the postern which points to this postern and the one this points to
    OBJ_DATA * prev(NULL);
    OBJ_DATA * next(NULL);
    for (size_t i(0); i < posterns.size(); ++i)
    {
        // Skip the target postern
        if (posterns[i] == postern)
            continue;

        // If the curr postern points to the target postern, the curr postern is prev
        if (posterns[i]->value[3] == postern->in_room->vnum)
        {
            if (prev != NULL) bug("remove_postern: duplicate candidates for previous postern", 0);
            prev = posterns[i];
        }

        // If the curr postern is pointed to by this postern, the curr postern is next
        if (postern->value[3] == posterns[i]->in_room->vnum)
        {
            if (next != NULL) bug("remove_postern: duplicate candidates for next postern", 0);
            next = posterns[i];
        }
    }

    // Point prev at next, if possible
    if (prev != NULL)
    {
        if (next == NULL) prev->value[3] = prev->in_room->vnum;
        else prev->value[3] = next->in_room->vnum;
    }
}

bool spell_forgepostern(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    // Sanity check
    if (ch->in_room == NULL)
    {
        bug("Attempted to forge postern in NULL room.", 0);
        return false;
    }

    // Check for norecall
    if (IS_SET(ch->in_room->room_flags, ROOM_NO_RECALL))
    {
        send_to_char("The curse upon this place inhibits such magics.\n", ch);
        return false;
    }

    // Check for an existing postern in this room
    for (OBJ_DATA * obj(ch->in_room->contents); obj != NULL; obj = obj->next_content)
    {
        if (obj->pIndexData->vnum == OBJ_VNUM_POSTERN)
        {
            send_to_char("The power of the postern already here would interfere with your own.\n", ch);
            return false;
        }
    }

    // Get some quintessence
    OBJ_DATA * quintessence(require_quintessence(ch));
    if (quintessence == NULL)
        return false;

    // Create the crystal
    OBJ_INDEX_DATA * posternIndex(get_obj_index(OBJ_VNUM_POSTERN));
    if (posternIndex == NULL)
    {
        send_to_char("An error has occurred; please inform the gods.\n", ch);
        bug("Unable to find object index for forge postern", 0);
        return false;
    }

    OBJ_DATA * postern(create_object(posternIndex, 0));
    if (postern == NULL)
    {
        send_to_char("An error has occurred; please inform the gods.\n", ch);
        bug("Unable to create postern object", 0);
        return false;
    }

    // Set the destination by looking up all existing posterns keyed to this caster
    OBJ_DATA * lastPostern(NULL);
    OBJ_DATA * firstPostern(NULL);
    std::vector<OBJ_DATA*> posterns(find_posterns_for(ch->id));
    for (size_t i(0); i < posterns.size(); ++i)
    {
        // The one with greatest level is the last one, the one with least level is the first one
        if (lastPostern == NULL || posterns[i]->level > lastPostern->level) lastPostern = posterns[i];
        if (firstPostern == NULL || posterns[i]->level < firstPostern->level) firstPostern = posterns[i];
    }

    // Key this postern to this caster
    AFFECT_DATA af = {0};
    af.type     = sn;
    af.level    = level;
    af.where    = TO_OBJECT;
    af.duration = -1;
    af.modifier = ch->id;
    affect_to_obj(postern, &af);

    // Point the new postern to the first postern (or loop back to this room if no first)
    if (firstPostern == NULL) postern->value[3] = ch->in_room->vnum;
    else postern->value[3] = firstPostern->in_room->vnum;

    // Now point the last postern to this postern, if there is a last. Also set the level.
    if (lastPostern == NULL) postern->level = 0;
    else 
    {
        postern->level = lastPostern->level + 1;
        lastPostern->value[3] = ch->in_room->vnum;
    }

    postern->timer = number_range(level * 10, level * 20);
    obj_to_room(postern, ch->in_room);

    // Check for positive power to save the essence
    int positivePower(Weave::AbsolutePositivePower(*ch->in_room));
    if (number_percent() <= 10 * positivePower)
        act("You draw on the Weave, gathering up the positive energy of this place to forge a pathway through the fabric of reality!", ch, NULL, NULL, TO_CHAR);
    else
    {
        act("You draw on the stored power, channeling its energy to forge a pathway through the fabric of reality!", ch, NULL, NULL, TO_CHAR);
        consume_quintessence(ch, quintessence);
    }

    act("$n traces an archway with $s hand, and silverly light slowly forms into a shimmering portal!", ch, NULL, NULL, TO_ROOM);
    return true;
}

bool spell_sealpostern(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    // Sanity-check
    if (ch->in_room == NULL)
    {
        bug("seal postern called in null room", 0);
        return false;
    }

    // Check that the caster is at one of his posterns
    std::vector<OBJ_DATA*> posterns(find_posterns_for(ch->id));
    OBJ_DATA * postern(NULL);
    for (size_t i(0); i < posterns.size(); ++i)
    {
        if (posterns[i]->in_room == ch->in_room)
        {
            postern = posterns[i];
            break;
        }
    }

    if (postern == NULL)
    {
        send_to_char("You must be in the presence of one of your posterns to seal any of them.\n", ch);
        return false;
    }

    act("You bring your hands together, closing them over each other slowly as you channel your will towards $p.", ch, postern, NULL, TO_CHAR);
    act("$n brings $s hands together, closing them over each other slowly.", ch, postern, NULL, TO_ROOM);

    // Check for whether this is all posterns
    if (target_name[0] != '\0' && !str_prefix(target_name, "all"))
    {
        // Seal all the posterns
        for (size_t i(0); i < posterns.size(); ++i)
            extract_obj(posterns[i]);

        if (posterns.size() > 1)
            send_to_char("You sense the Weave shift from afar as all your posterns seal shut.\n", ch);

        return true;
    }

    // Seal just this postern
    extract_obj(postern);
    return true;
}

bool spell_dreamshape(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    char arg[MAX_STRING_LENGTH];
    char * argument(one_argument(target_name, arg));
    
    // Lookup the target
    CHAR_DATA * victim(get_char_room(ch, arg));
    if (victim == NULL)
    {
        send_to_char("You don't see anyone here by that name.\n", ch);
        return false;
    }

    if (victim == ch)
    {
        send_to_char("You do not need magic to shape your own dreams.\n", ch);
        return false;
    }

    // Make sure the target is sleeping and that an argument was specified
    if (IS_AWAKE(victim))
    {
        act("How do you propose to shape $N's dreams when $E is awake?", ch, NULL, victim, TO_CHAR);
        return false;
    }

    argument = one_argument(argument, arg);
    if (arg[0] == '\0')
    {
        act("How would you like to shape $S dreams?", ch, NULL, victim, TO_CHAR);
        return false;
    }
    
    // Determine the desired action
    if (!str_prefix(arg, "weave"))
    {
        if (argument[0] == '\0')
        {
            act("What would you like to weave into $N's dreams?", ch, NULL, victim, TO_CHAR);
            return false;
        }

        // Send the echo to the target
        act_new(argument, victim, NULL, NULL, TO_CHAR, POS_DEAD);
        act("You weave images into $N's dreams, shaping them with your will.", ch, NULL, victim, TO_CHAR);
        return true;
    }

    // The remaining effects require targets to not be safe from the caster's spells
    if (is_safe_spell(ch, victim, false))
    {
        act("You are unable to shape $S dreams.", ch, NULL, victim, TO_CHAR);
        return false;
    }

    // Determine type
    enum Type {Hibernate, Visions};
    Type type;
    if (!str_prefix(arg, "hibernate")) type = Hibernate;
    else if (!str_prefix(arg, "visions")) type = Visions;
    else
    {
        send_to_char("That is not a valid way of shaping dreams.\n", ch);
        return false;
    }

    // Check for a save
    if (saves_spell(level + lethebane_sleep_level_mod(ch, victim), ch, victim, DAM_MENTAL))
    {
        act("$N resists your attempts to shape $S dreams.", ch, NULL, victim, TO_CHAR);
        send_to_char("Your dreams grow strange and unfamiliar, but you push them away with an effort of will.\n", victim);
        return true;
    }

    // Check for hibernate
    if (type == Hibernate)
    {
        if (IS_AFFECTED(victim, AFF_SLEEP))
        {
            act("$N is already sleeping deeply.", ch, NULL, victim, TO_CHAR);
            return false;
        }

        act("$n begins to breathe more deeply as $e sinks into a heavier sleep.", victim, NULL, NULL, TO_ROOM);
        send_to_char("Your dreams grow slow and hazier, lulling you into a much deeper repose.\n", victim);

        // Add the sleep effect
        AFFECT_DATA af = {0};
        af.where    = TO_AFFECTS;
        af.type     = gsn_sleep;
        af.level    = level;
        af.duration  = 3;
        af.bitvector = AFF_SLEEP;
        affect_to_char(victim, &af);
        return true;
    }

    // Check for visions
    if (type == Visions)
    {
        if (is_affected(victim, gsn_visions))
        {
            act("$N is already tormented by visions of damnation.", ch, NULL, victim, TO_CHAR);
            return false;
        }

        // Goodly folk are immune
        if (effective_karma(*victim) <= PALEGOLDENAURA)
        {
            act("$N's conscience is too clean for such visions to be effective.", ch, NULL, victim, TO_CHAR);
            return false;
        }

        act("$n grows restless in $s sleep as $e begins to toss and turn.", victim, NULL, NULL, TO_ROOM);
        send_to_char("You grow restless in your sleep as your dreams turn to nightmares of your own damnation!\n", victim);

        // Apply the visions
        AFFECT_DATA af = {0};
        af.where    = TO_AFFECTS;
        af.type     = gsn_visions;
        af.level    = level;
        af.duration = (level / 8);
        affect_to_char(victim, &af);
        return true;
    }

    // Unknown argument
    send_to_char("An error has occurred, please contact the gods.\n", ch);
    bug("dreamshape invalid argument slipped through", 0);
    return false;
}

void tryApplyPacification(int level, int duration, CHAR_DATA * ch, CHAR_DATA * victim, bool areaWide)
{
    // Check for auto-fail; undead have no spirits to calm
    if (IS_NPC(victim) && (IS_SET(victim->act, ACT_UNDEAD) || IS_SET(victim->act, ACT_NOSUBDUE)))
        return;

    // Berserking and frenzied folk are immune, and so is the caster; check for save
    if (IS_AFFECTED(victim, AFF_BERSERK) || is_affected(victim, gsn_frenzy) || ch == victim || is_safe_spell(ch, victim, areaWide) || saves_spell(level, ch, victim, DAM_OTHER))
        return;

    // Failed save; stop fighting
    send_to_char("As the silver-pearl light washes over you, all your being is overcome with a sense of serenity.\n", victim);
    act("$n grows calm as the silver-pearl light washes over $m.", victim, NULL, NULL, TO_ROOM);
    
    victim->tracking = NULL;
    if (victim->fighting != NULL)
        stop_fighting_all(victim);

    // Check for already affected; do not refresh the effect
    if (is_affected(victim, gsn_pacify))
        return;

    // Add the effect
    AFFECT_DATA af = {0};
    af.where    = TO_NAFFECTS;
    af.type     = gsn_pacify;
    af.level    = level;
    af.duration = duration;
    af.bitvector = AFF_SUBDUE;
    affect_to_char(victim, &af);
}

bool spell_pacify(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    // Sanity check
    if (ch->in_room == NULL)
        return false;

    // Check for scope
    if (target_name[0] == '\0')
    {
        // No argument provided; scope is the room
        act("You spread your arms wide, and a gentle wave of opalescent light flows out from you.", ch, NULL, NULL, TO_CHAR);
        act("$n spreads $s arms wide, and a gentle wave of opalescent light flows out from $m.", ch, NULL, NULL, TO_ROOM);
        
        int effectiveLevel(level - 1);
        for (CHAR_DATA * victim(ch->in_room->people); victim != NULL; victim = victim->next_in_room)
            tryApplyPacification(--effectiveLevel, level / 11, ch, victim, true);

        return true;
    }
    
    if (!str_prefix(target_name, "area"))
    {
        // Area targeted; get PCs in area
        act("You raise your arms to the heavens, and gentle waves of pearly-grey light begin to spread through the area.", ch, NULL, NULL, TO_CHAR);
        act("$n raises $s arms to the heavens, and gentle waves of pearly-grey light begin to spread through the area.", ch, NULL, NULL, TO_ROOM);
        
        int effectiveLevel(level - 3);
        for (DESCRIPTOR_DATA * d(descriptor_list); d->next != NULL; d = d->next)
        {

            // Filter out certain characters
            if (d->connected != CON_PLAYING || d->character->in_room == NULL
            ||  d->character->in_room->area != ch->in_room->area)
                continue;

            // PC is in the same area as the caster
            if (d->character->in_room != ch->in_room)
                send_to_char("A gentle wave of pearly-grey light washes over the area.\n", d->character);

            tryApplyPacification(--effectiveLevel, level / 17, ch, d->character, true);
        }

        return true;
    }

    // Lookup the specified target
    CHAR_DATA * victim(get_char_room(ch, target_name));
    if (victim == NULL)
    {
        send_to_char("You see no one here by that name.\n", ch);
        return false;
    }

    act("You raises a hand, willing a gentle wave of silvery light to issue towards $N.", ch, NULL, victim, TO_CHAR);
    act("$n raises a hand, and a gentle wave of silvery light issues from it towards you.", ch, NULL, victim, TO_VICT);
    act("$n raises a hand, and a gentle wave of silvery light issues from it towards $N.", ch, NULL, victim, TO_NOTVICT);
    tryApplyPacification(level, level / 8, ch, victim, false);
    return true;
}

static std::vector<AFFECT_DATA *> findHauntings(CHAR_DATA * ch)
{
    std::vector<AFFECT_DATA *> result;
    for (AFFECT_DATA * paf(ch->affected); paf != NULL; paf = paf->next)
    {
        if (paf->type == gsn_shadeswarm || paf->point == &gsn_shadeswarm)
            result.push_back(paf);
    }

    return result;
}

bool spell_absolvespirit(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    // Find a target
    CHAR_DATA * victim;
    if (target_name[0] == '\0')
    {
        if (ch->fighting == NULL)
        {
            send_to_char("Whom did you wish to absolve?\n", ch);
            return false;
        }

        victim = ch->fighting;
    }
    else
    {
        victim = get_char_room(ch, target_name);
        if (victim == NULL)
        {
            send_to_char("You see no one here by that name.\n", ch);
            return false;
        }
    }

    // Check for a cooldown on the target
    if (is_affected(victim, sn))
    {
        send_to_char("Your target has already been absolved recently.\n", ch);
        return false;
    }

    // Check whether the target is a shade
    if (IS_NPC(victim) && IS_SET(victim->nact, ACT_SHADE))
    {
        // Shade found, absolve it
        act("You murmur a word of absolution, and $N dissipates back into the aether.", ch, NULL, victim, TO_CHAR);
        act("$n murmurs an arcane word, and you shiver at a sudden haunting chill which quickly passes.", ch, NULL, victim, TO_ROOM);

        int bonus(victim->level * 2);
        if (perform_litany_skill_check(ch, victim, Litany_Purification, 1))
            bonus *= 2;

        ch->mana = UMIN(ch->max_mana, (ch->mana + bonus));
        extract_char(victim, true);
        return true;
    }

    int purificationMod(perform_litany_skill_check(ch, victim, Litany_Purification, 1) ? 6 : 5);

    // Check whether the target is a spirit or demon
    if (victim->race == race_lookup("spirit") || is_demon(victim))
    {
        act("You murmur a word of absolution, holding a palm out towards $N.", ch, NULL, victim, TO_CHAR);
        act("$n murmurs a word of absolution, holding a palm out towards you.", ch, NULL, victim, TO_VICT);
        act("$n murmurs a word of absolution, holding a palm out towards $N.", ch, NULL, victim, TO_NOTVICT);

        // Treat the target differently by alignment
        // Evil spirits are damaged with no chance for save
        if (IS_TRUE_EVIL(victim)) 
            damage_old(ch, victim, (dice(level, 15) * purificationMod) / 5, sn, DAM_HOLY, true);
        else
        {
            // Other spirits are healed; this effect is particularly strong for good spirits
            int healAmount((dice(level, (IS_TRUE_GOOD(victim) ? 19 : 11)) * purificationMod) / 5);
            victim->hit = UMIN(victim->max_hit, victim->hit + healAmount);
            act("You feel the weight of past faults drop away, and gaze upon the world with renewed vitality!", victim, NULL, NULL, TO_CHAR);
            act("$n glows briefly with a soft white light, its purity seeming to lend $m a renewed vitality.", victim, NULL, NULL, TO_ROOM);
        }

        // Add the cooldown
        AFFECT_DATA af = {0};
        af.where    = TO_AFFECTS;
        af.type     = sn;
        af.level    = level;
        af.duration = 120;
        affect_to_char(victim, &af);
        return true;
    }

    // Look for hauntings
    std::vector<AFFECT_DATA *> hauntings(findHauntings(victim));
    if (hauntings.empty())
    {
        send_to_char("You can only absolve shades, spirits, demons, or haunted mortals.\n", ch);
        return false;
    }

    act("You murmur a word of absoution, holding a palm out towards $N.", ch, NULL, victim, TO_CHAR);
    act("$n murmurs a word of absolution, holding a palm out towards you.", ch, NULL, victim, TO_VICT);
    act("$n murmurs a word of absolution, holding a palm out towards $N.", ch, NULL, victim, TO_NOTVICT);
    act("The spirits haunting $M dissipate, absolved by your power.", ch, NULL, victim, TO_CHAR);
    act("Your back unclenches as the spirits haunting you dissipate, absolved by $n's power.", ch, NULL, victim, TO_VICT);

    for (size_t i(0); i < hauntings.size(); ++i)
        affect_remove(victim, hauntings[i]);

    return true;
}

bool spell_unweave(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    // Check for arguments
    if (target_name[0] == '\0')
    {
        send_to_char("What magics did you wish to unweave?\n", ch);
        return false;
    }

    // Try to find some quintessence
    OBJ_DATA * obj(require_quintessence(ch));
    if (obj == NULL) 
        return false;

    // Grab the first argument
    char arg[MAX_STRING_LENGTH];
    char * argument(one_argument(target_name, arg));
    CHAR_DATA * victim(get_char_room(ch, arg));
    if (victim == NULL)
    {
        // No person targeted, use the caster as target
        victim = ch;
        argument = target_name;
    }
    else if (!is_same_group(ch, victim))
    {
        send_to_char("Only a groupmate would cooperate long enough with you to complete the unweaving.\n", ch);
        return false;
    }

    // Check for empty target spell
    if (argument[0] == '\0')
    {
        send_to_char("What magics did you wish to unweave?\n", ch);
        return false;
    }

    // Lookup the spell
    int targetSN(skill_lookup(argument));
    if (targetSN < 0)
    {
        std::ostringstream mess;
        mess << "There are no Weavings known by the name '" << argument << "'.\n";
        send_to_char(mess.str().c_str(), ch);
        return false;
    }

    // Make sure the target is affected by the SN
    AFFECT_DATA * paf(get_affect(victim, targetSN));
    if (paf == NULL)
    {
        if (ch == victim) act("You are not bound by such a Weaving.", ch, NULL, NULL, TO_CHAR);
        else act("$N is not bound by such a Weaving.", ch, NULL, victim, TO_CHAR);
        return false;
    }

    // Make sure it is unweaveable
    if (!IS_SET(skill_table[targetSN].flags, SN_UNWEAVABLE))
    {
        send_to_char("Such a binding cannot be unwoven.\n", ch);
        return false;
    }

    // Unreal incursion prevents this
    if (is_affected(ch, gsn_unrealincursion))
    {
        send_to_char("You cannot discern which threads of quintessence are real, and which are illusory!\n", ch);
        return false;
    }

    act("You trace an arcane symbol in the air with one hand; slowly, a golden light begins to form in the palm of your other.", ch, NULL, NULL, TO_CHAR);
    act("$n traces an arcane symbol in the air with one hand; slowly, a golden light begins to form in the palm of $s other.", ch, NULL, NULL, TO_ROOM);

    if (ch == victim)
    {
        act("You suddenly shout a word of power, releasing the golden light! It pours into you, making your body glow briefly.", ch, NULL, NULL, TO_CHAR);
        act("$n suddenly shouts a word of power, releasing the golden light! It pours into $m, making $s body glow briefly.", ch, NULL, NULL, TO_ROOM);
    }
    else
    {
        act("You suddenly shout a word of power, releasing the golden light! It pours into $N, making $S body glow briefly.", ch, NULL, victim, TO_CHAR);
        act("$n suddenly shouts a word of power, releasing the golden light! It pours into you, making your body glow briefly.", ch, NULL, victim, TO_VICT);
        act("$n suddenly shouts a word of power, releasing the golden light! It pours into $N, making $S body glow briefly.", ch, NULL, victim, TO_NOTVICT);
    }

    // Check for a save
    std::ostringstream mess;
    if (saves_dispel(level, paf->level, 0))
    {
        // Saved; reduce the level of the spell
        paf->level = UMAX(1, paf->level - 10);
        mess << "You feel the power of the " << skill_table[targetSN].name << " working upon you diminish, but remain intact.\n";
        send_to_char(mess.str().c_str(), victim);

        if (ch != victim)
        {
            mess.str("");
            mess << "You feel the power of the " << skill_table[targetSN].name << " working diminish, but remain intact.\n";
            send_to_char(mess.str().c_str(), ch);
        }
    }
    else
    {
        // Did not save; remove the affect
        affect_strip(victim, targetSN);
        mess << "You carefully unweave the " << skill_table[targetSN].name << " working, unravelling its power completely.\n";
        send_to_char(mess.str().c_str(), ch);

        if (ch != victim)
        {
            mess.str("");
            mess << "You feel the power of the " << skill_table[targetSN].name << " working upon you lift, unravelling into nothingness.\n";
            send_to_char(mess.str().c_str(), victim);
        }
    }

    // Destroy the quintessence
    consume_quintessence(ch, obj);
    return true;
}

void handle_quintessence_destruction(CHAR_DATA * ch, OBJ_DATA * obj)
{
    // Check for quintessence
    if (!IS_OBJ_STAT_EXTRA(obj, ITEM_QUINTESSENCE))
        return;
    
    act("Thin wisps of glowing white fog swirl up from $p, vanishing as you breathe them in.", ch, obj, NULL, TO_CHAR);
    act("Thin wisps of glowing white fog swirl up from $p, vanishing as $n breathes them in.", ch, obj, NULL, TO_ROOM);

    // Check for cooldown
    AFFECT_DATA * paf(NULL);
    for (paf = get_affect(ch, gsn_crystallizeaether); paf != NULL; paf = get_affect(ch, gsn_crystallizeaether, paf))
    {
        if (paf->modifier > 0)
            break;
    }

    int bonusMana(obj->level * 7);
    if (paf == NULL)
    {
        // Add a cooldown
        AFFECT_DATA af = {0};
        af.where    = TO_AFFECTS;
        af.type     = gsn_crystallizeaether;
        af.level    = ch->level;
        af.modifier = 1;
        af.duration = 6;
        affect_to_char(ch, &af);
    }
    else
    {
        // Reset cooldown and reduce the effect
        ++paf->modifier;
        paf->duration = 6;
        bonusMana /= paf->modifier;
    }

    // Grant the bonus mana
    send_to_char("You feel invigorated as you breathe in the living mists.\n", ch);
    ch->mana = UMIN(ch->max_mana, ch->mana + bonusMana);
}

bool spell_crystallizeaether(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    // Check for cooldown
    for (AFFECT_DATA * paf(get_affect(ch, sn)); paf != NULL; paf = get_affect(ch, sn, paf))
    {
        if (paf->modifier == 0)
        {
            send_to_char("You are not yet prepared to bind the aether again.\n", ch);
            return false;
        }
    }

    // Check for ley line or fount
    if (ch->in_room == NULL || ch->in_room->ley_group == NULL || (!ch->in_room->ley_group->HasFount() && ch->in_room->ley_group->LineCount() < 1))
    {
        send_to_char("The Weave is not strong enough here for such magics.\n", ch);
        return false;
    }

    // Create the crystal
    OBJ_INDEX_DATA * crystalIndex(get_obj_index(OBJ_VNUM_QUINTESSENCE));
    if (crystalIndex == NULL)
    {
        send_to_char("An error has occurred; please inform the gods.\n", ch);
        bug("Unable to find object index for crystallize aether", 0);
        return false;
    }

    OBJ_DATA * crystal(create_object(crystalIndex, 0));
    if (crystal == NULL)
    {
        send_to_char("An error has occurred; please inform the gods.\n", ch);
        bug("Unable to create quintessence object", 0);
        return false;
    }

    // Set the crystal level and give it to the caster
    crystal->level = level;
    obj_to_char(crystal, ch);

    // Apply cooldown
    AFFECT_DATA af = {0};
    af.where    = TO_AFFECTS;
    af.type     = sn;
    af.level    = level;
    af.duration = 80 - UMAX(0, (get_skill(ch, sn) - 70) / 3);
    affect_to_char(ch, &af);

    // Send echoes
    act("With a force of will, you draw shining tendrils of Quintessence from the Weave, binding them into a tiny, sparkling crystal.", ch, NULL, NULL, TO_CHAR);
    act("$n concentrates, and a ball of pure white light forms in $s hands. When the light fades, only a tiny, sparkling crystal remains.", ch, NULL, NULL, TO_ROOM);
    return true;
}

static bool has_aggro_character(const ROOM_INDEX_DATA & room, CHAR_DATA & target)
{
    for (CHAR_DATA * aggro(room.people); aggro != NULL; aggro = aggro->next_in_room)
    {
        if (should_aggro(aggro, &target))
            return true;
    }

    return false;
}

bool spell_leapoffaith(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    // Make sure the room is ok for teleporting
    if (ch->in_room == NULL || IS_SET(ch->in_room->room_flags, ROOM_NO_RECALL)
    || IS_SET(ch->in_room->room_flags, ROOM_NOGATE) || is_affected(ch, gsn_matrix)
    || (ch->in_room->area->area_flags & AREA_UNCOMPLETE) || IS_AFFECTED(ch, AFF_CURSE))
    {
        send_to_char("You murmur a quick prayer, but cannot find a path clear of this place.\n", ch);
        return false;
    }

    // Verify sufficient move
    if (ch->move <= 0)
    {
        send_to_char("You are too tired to take a leap of faith right now.\n", ch);
        return false;
    }

    // Unreal incursion prevents leap of faith
    if (is_affected(ch, gsn_unrealincursion))
    {
        send_to_char("Your reality is too distorted to summon such faith.\n", ch);
        return false;
    }

    // Determine save odds
    std::vector<int> chances;
    int aura_level(aura_grade(ch));
    switch (aura_level)
    {
        case 0:  chances.push_back(50);  break;
        case -1: chances.push_back(75);  break;
        case -2: chances.push_back(100); break;
        case -3: chances.push_back(100); chances.push_back(33); break;
        case -4: chances.push_back(100); chances.push_back(67); break;
        default:
            if (aura_level < -4)
            {
                chances.push_back(100);
                chances.push_back(67);
                bug("Found invalid aura grade in leap of faith", 0);
            }
            break;
    }

    // Echo about the leap
    act("With a murmured prayer, you draw the Weave about you and leap through it!", ch, NULL, NULL, TO_CHAR);
    act("$n murmurs a quick prayer, then vanishes in a burst of white light!", ch, NULL, NULL, TO_ROOM);
    ch->move /= 2;

    unsigned int i(0);
    while (true)
    {
        ROOM_INDEX_DATA * room(get_random_room(ch));

        // Bring along any familiar
        if (ch->familiar != NULL && ch->familiar->in_room == ch->in_room)
        {
            act("$n vanishes in a burst of white light!", ch->familiar, NULL, NULL, TO_ROOM);
            char_from_room(ch->familiar);
            char_to_room(ch->familiar, room);
            act("$n slowly fades into existence.", ch->familiar, NULL, NULL, TO_ROOM);
            do_look(ch->familiar, "auto");
        }

        // Perform the leap
        char_from_room(ch);
        char_to_room(ch, room);
        act("A pulse of white light illuminates the area, slowly resolving into the shining form of $n.", ch, NULL, NULL, TO_ROOM);
        do_look(ch, "auto");

        // If no aggro here, just bail out
        if (!has_aggro_character(*room, *ch))
            break;

        // Aggro exists; check for a save
        if (chances.size() <= i || number_percent() > chances[i])
        {
            // Failed to save
            send_to_char("You sense danger, and lose faith as fear fills you!\n", ch);
            break;
        }

        // Faith held, pick a new random room
        send_to_char("You sense danger, but hold firm to your faith! You feel a holy power envelop you, spiriting you away from harm!\n", ch);
        act("$n glows with a pure white light, then vanishes again just as quickly as $e arrived!", ch, NULL, NULL, TO_ROOM);
        ++i;
    }        

    return true;
}

bool spell_phaseshift(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    // Build the affect
    AFFECT_DATA af = {0};
    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = number_fuzzy(level / 4);
    af.location  = APPLY_RESIST_WEAPON;
    af.modifier  = (level / 10);
    af.bitvector = AFF_PASS_DOOR;

    for (CHAR_DATA * vch(ch->in_room->people); vch != NULL; vch = vch->next_in_room)
    {
        if (is_same_group(ch, vch))
        {
            // Replace any existing phaseshift
            act("You shift phase, and the physical world seems to grow slightly hazier.", vch, NULL, NULL, TO_CHAR);
            act("$n shimmers, becoming slightly translucent.", vch, NULL, NULL, TO_ROOM);
            affect_strip(vch, sn);
            affect_to_char(vch, &af);
        }
    }

    return true;
}

bool spell_searinglight(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;

    bool saved(false);
    int dam = 27 + ((level * 2) / 3);
    dam = number_range(dam / 2, dam * 2);

    // Check for litany of mortification; if not present, check for saves
    if (perform_litany_skill_check(ch, victim, Litany_Mortification, 5))
        act("You mortify $N with your magic, bypassing $S defenses!", ch, NULL, victim, TO_CHAR);
    else if (saves_spell(level, ch, victim, DAM_LIGHT))
    {
        dam = dam / 2;
        saved = true;
    }

    // Handle quintessence rush boost
    if (is_quintessence_rushing(ch))
    {
        dam = (dam * 5) / 4;
        if (!saved && number_bits(1) == 0)
        {
            act("The searing light flares a bright white, raking across your vision!", victim, NULL, NULL, TO_CHAR);
            act("The searing light flares a bright white, raking across $n's vision!", victim, NULL, NULL, TO_ROOM);
            
            if (!IS_AFFECTED(victim, AFF_BLIND) && !IS_SET(victim->imm_flags, IMM_BLIND))
            {
                if (is_affected(victim, gsn_reflectiveaura))
                {
                    act("As the light reaches you, it is dispersed by your reflective aura.", victim, NULL, NULL, TO_CHAR);
                    act("As the light reaches $n, it is dispersed by $s reflective aura.", victim, NULL, NULL, TO_ROOM);
                }
                else
                {
                    act("The world goes dark as your eyes burn from the light!", victim, NULL, NULL, TO_CHAR);
                    act("$n is blinded by the dazzling light!", victim, NULL, NULL, TO_ROOM);

                    // Apply the blindness
                    AFFECT_DATA af = {0};
                    af.where     = TO_AFFECTS;
                    af.type      = gsn_blindness;
                    af.level     = level;
                    af.location  = APPLY_HITROLL;
                    af.modifier  = -4;
                    af.duration  = 1 + (level / 17);
                    af.bitvector = AFF_BLIND;
                    affect_to_char(victim, &af);
                }
            }
        }
    }

    damage_old(ch, victim, dam, sn, DAM_LIGHT, true);
    return true;
}

static int calculateAegisModifier(CHAR_DATA * vch, int amount)
{
    int grade(aura_grade(vch));
    switch (grade)
    {
        case -4: amount += 20; break;
        case -3: amount += 19; break;
        case -2: amount += 18; break;
        case -1: amount += 17; break;
        case 0:  amount += 16; break;
        default:
            if (grade < -4)
            {
                amount += 20;
                bug("Invalid aura grade in calculateAegisModifier", 0);
            }
            break;
    }

    return amount;
}

bool spell_wardofgrace(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    // Check for existing
    if (is_affected(ch, gsn_aegisofgrace))
    {
        send_to_char("You are already warded by an aegis of grace.\n", ch);
        return false;
    }

    act("A beam of pure white light shines briefly upon you, filling you with a warm glow.", ch, NULL, NULL, TO_CHAR);
    act("A beam of pure white light shines briefly upon $n.", ch, NULL, NULL, TO_ROOM);

    // Litany of benediction is good for +5% reduction
    int bonus(0);
    if (perform_litany_skill_check(ch, NULL, Litany_Benediction, 1))
        bonus = 5;

    // Apply the effect
    AFFECT_DATA af = {0};
    af.where    = TO_AFFECTS;
    af.type     = gsn_aegisofgrace;
    af.level    = level;
    af.duration = 36;
    af.modifier = calculateAegisModifier(ch, bonus);
    affect_to_char(ch, &af);
    return true;
}

bool spell_aegisofgrace(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    AFFECT_DATA af = {0};
    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = 36 * (check_durablemagics(*ch) ? 2 : 1);

    // Litany of benediction is good for +5% reduction
    int bonus(0);
    if (perform_litany_skill_check(ch, NULL, Litany_Benediction, 1))
        bonus = 5;

    // Apply the aegis to all groupmates
    for (CHAR_DATA * vch(ch->in_room->people); vch != NULL; vch = vch->next_in_room)
    {
        if (is_same_group(ch, vch))
        {
            // Replace any existing aegis of grace
            act("A beam of pure white light shines briefly upon you, filling you with a warm glow.", vch, NULL, NULL, TO_CHAR);
            act("A beam of pure white light shines briefly upon $n.", vch, NULL, NULL, TO_ROOM);
            affect_strip(vch, sn);

            // Calculate the modifier according to alignment/karma
            af.modifier = calculateAegisModifier(vch, bonus);
            affect_to_char(vch, &af);
        }
    }

    return true;
}

bool spell_undyingradiance(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    // Check for object
    if (target_name[0] != '\0')
    {
        // Get the object
        OBJ_DATA * light(get_obj_carry(ch,target_name,ch));
        if (light == NULL)
        {
            send_to_char("You don't see that here.\n", ch);
            return false;
        }

        // Check whether already glowing
        if (IS_OBJ_STAT(light,ITEM_GLOW))
        {
            act("$p is already suffused with light.", ch, light, NULL, TO_CHAR);
            return false;
        }

        // Check for dim
        AFFECT_DATA * paf(affect_find(light->affected, gsn_dim));
        if (paf != NULL)
        {
            // Check for saves
            if (saves_dispel(level, paf->level, paf->duration))
            {
                act("You conjure wisps of light to suffuse $p, but cannot dispel the dimness within it.", ch, light, NULL, TO_CHAR);
                return true;
            }

            // Dim removed
            object_affect_strip(light, gsn_dim);
            act("You conjure wisps of light to suffuse $p, driving the dimness from it with force of will!", ch, light, NULL, TO_CHAR);
            return true;
        }

        // Dim not set, so add the glow flag
        SET_BIT(light->extra_flags[0], ITEM_GLOW);
        act("You conjure wisps of light to suffuse $p, and it shines with a white light.", ch, light, NULL, TO_CHAR);
        act("$n chants slowly, causing $p to shine with a white light.", ch, light, NULL, TO_ROOM);
        return true;
    }

    // Object not targeted, so look to the room
    AFFECT_DATA * paf(affect_find(ch->in_room->affected, gsn_dim));
    if (paf != NULL)
    {
        // Check for saves
        if (saves_dispel(level, paf->level, paf->duration))
        {
            act("You call forth a pure white light, but cannot overcome the dimness here.", ch, NULL, NULL, TO_CHAR);
            return true;
        }

        // Dim removed
        room_affect_strip(ch->in_room, gsn_dim);
        act("You call forth a pure white light, driving the dimness from this place.", ch, NULL, NULL, TO_CHAR);
        act("$n calls forth a pure white light, driving the dimness from this place.", ch, NULL, NULL, TO_ROOM);
        return true;
    }
    
    // No dimness, so make the room glow
    AFFECT_DATA af = {0};
    af.duration = (level * (check_durablemagics(*ch) ? 2 : 1) / 3);
    af.level = level;
    af.type = sn;
    af.where = TO_ROOM_AFF;
    affect_to_room(ch->in_room, &af);

    act("You chant, calling forth a pure white light to illuminate this place.", ch, NULL, NULL, TO_CHAR);
    act("$n chants, calling forth a pure white light to illuminate this place.", ch, NULL, NULL, TO_ROOM);
    return true;
}

bool spell_riteofablution(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    // Check the object case
    if (target == TARGET_OBJ)
    {
        OBJ_DATA * obj((OBJ_DATA *)vo);

        // Make sure there is a curse
        if (!IS_OBJ_STAT(obj,ITEM_NODROP) && !IS_OBJ_STAT(obj, ITEM_NOREMOVE))
        {
            act("You can sense no curse on $p.", ch, obj, NULL, TO_CHAR);
            return false;
        }

        // Prevent uncursing of sigils and nouncurse items
        if (obj->wear_flags & ITEM_WEAR_SIGIL || IS_OBJ_STAT(obj, ITEM_NOUNCURSE))
        {
            act("You cannot wash away the stain of $p.", ch, obj, NULL, TO_CHAR);
            return false;
        }
        
        // Determine the level of the curse
        int dispelLevel(obj->level - (perform_litany_skill_check(ch, NULL, Litany_Purification, 1) ? 5 : 0));
        if (obj_is_affected(obj, gsn_cursebarkja) && !room_is_affected(ch->in_room, gsn_sanctify))
            dispelLevel = UMAX(dispelLevel, 70);

        act("You raise your palm towards $p, solemnly intoning the words of the Rite.", ch, obj, NULL, TO_CHAR);
        act("$n raises $s palm towards $p, solemnly intoning an arcane phrase.", ch, obj, NULL, TO_ROOM);
        
        // Check for a save
        if (saves_dispel(level, dispelLevel, 0))
        {
            act("The curse within $p twists at your rebuke, but holds fast against your power.", ch, obj, NULL, TO_CHAR);
            return true;
        }

        // Curse is dispelled
        act("Dark mists seep out of $p as its curse departs, rebuked by your power.", ch, obj, NULL, TO_CHAR);
        act("Dark mists seep out of $p, seeming to recoil from $n even as they dissipate.", ch, obj, NULL, TO_ROOM);
        REMOVE_BIT(obj->extra_flags[0], ITEM_NODROP);
        REMOVE_BIT(obj->extra_flags[0], ITEM_NOREMOVE);

        return true;
    }

    // Check the character case
    CHAR_DATA * victim((CHAR_DATA *)vo);
    if (ch == victim)
    {
        act("You raise your palm, solemnly intoning the words of the Rite.", ch, NULL, NULL, TO_CHAR);
        act("$n raises $s palm, solemnly intoning the words of the Rite.", ch, NULL, NULL, TO_ROOM);
    }
    else
    {
        act("You raise your palm towards $N, solemnly intoning the words of the Rite.", ch, NULL, victim, TO_CHAR);
        act("$n raises $s palm towards you, solemnly intoning an arcane phrase.", ch, NULL, victim, TO_VICT);
        act("$n raises $s palm towards $N, solemnly intoning an arcane phrase.", ch, NULL, victim, TO_NOTVICT);
    }

    // Check for success on the target
    if (check_dispel(level + (perform_litany_skill_check(ch, victim, Litany_Purification, 1) ? 5 : 0), victim, gsn_curse))
    {
        if (ch == victim)
        {
            act("Dark mists seep out of you as your curse departs, rebuked by your power.", ch, NULL, NULL, TO_CHAR);
            act("Dark mists seep out of $n, seeming to recoil from $m even as they dissipate.", ch, NULL, NULL, TO_ROOM);
        }
        else
        {
            act("Dark mists seep out of $N as $S curse departs, rebuked by your power.", ch, NULL, victim, TO_CHAR);
            act("Dark mists seep out of you as your curse departs, rebuked by $n's power.", ch, NULL, victim, TO_VICT);
            act("Dark mists seep out of $N, seeming to recoil from $n even as they dissipate.", ch, NULL, victim, TO_NOTVICT);
        }
        return true;
    }
    
    // Check for success on target's objects
    for (OBJ_DATA * obj(victim->carrying); obj != NULL; obj = obj->next_content)
    {
        // Check for a curse
        if (!IS_OBJ_STAT(obj, ITEM_NODROP) && !IS_OBJ_STAT(obj, ITEM_NOREMOVE))
            continue;

        // Check for uncursability
        if (IS_OBJ_STAT(obj, ITEM_NOUNCURSE) || IS_SET(obj->wear_flags, ITEM_WEAR_SIGIL) || IS_SET(obj->wear_flags, ITEM_PROG))
            continue;

        // Determine the level of the curse
        int dispelLevel(obj->level - (perform_litany_skill_check(ch, victim, Litany_Purification, 1) ? 5 : 0));
        if (obj_is_affected(obj, gsn_cursebarkja) && !room_is_affected(ch->in_room, gsn_sanctify))
            dispelLevel = UMAX(dispelLevel, 70);

        // Check for saves
        if (saves_dispel(level, dispelLevel, 0))
            continue;
          
        // Curse is removed   
        REMOVE_BIT(obj->extra_flags[0], ITEM_NODROP);
        REMOVE_BIT(obj->extra_flags[0], ITEM_NOREMOVE);
        act("Dark mists seep out of $p as its curse departs, rebuked by your power.", ch, obj, NULL, TO_CHAR);
        act("Dark mists seep out of $p, seeming to recoil from $n even as they dissipate.", ch, obj, NULL, TO_ROOM);
    }

    return true;
}

bool spell_bondofsouls(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *)vo;

    if (!is_same_group(ch, victim) && victim->leader != ch)
    {
        act("$E must be grouped with you or following you.", ch, NULL, victim, TO_CHAR);
        return false;
    }

    if (ch == victim)
    {
        send_to_char("You cannot form a bond to your own soul.\n", ch);
        return false;
    }

    if (is_affected(victim, sn))
    {
        act("$S soul is already linked to another.", ch, NULL, victim, TO_CHAR);
        return false;
    }

    if (is_affected(ch, sn))
    {
        send_to_char("Your soul is already linked to another.\n", ch);
        return false;
    }

    act("You touch two fingers lightly to $N's temple, and feel a subtle but deep link form between you.", ch, NULL, victim, TO_CHAR);        
    act("$n touches two fingers lightly against your temple, and you feel a subtle but deep link form between you.", ch, NULL, victim, TO_VICT);        
    act("$n touches two fingers lightly against $N's temple, murmuring a soft incantation.", ch, NULL, victim, TO_NOTVICT);

    AFFECT_DATA af = {0};
    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = (level * (check_durablemagics(*ch) ? 2 : 1)) / 2;
    af.location  = APPLY_HIDE;
    af.modifier  = ch->id;
    affect_to_char( victim, &af );

    af.modifier  = victim->id;
    affect_to_char( ch, &af );

    af.location  = APPLY_HIT;
    af.modifier  = level*2;
    affect_to_char( victim, &af );
    victim->hit += level*2;
    
    affect_to_char( ch, &af );
    ch->hit += level*2;

    af.location  = APPLY_MANA;
    af.modifier  = level*2;
    affect_to_char( victim, &af );
    victim->mana += level*2;
    
    affect_to_char( ch, &af );
    ch->mana += level*2;
    return true;
}

void do_weavesense(CHAR_DATA * ch, char *)
{
    // Avatars cannot change state of weavesense
    if (is_an_avatar(ch))
    {
        send_to_char("In this state the weave flows through your very being! You cannot help but sense it!\n", ch);
        return;
    }

    // Anyone can turn it off, so check whether it is on
    AFFECT_DATA * paf(get_affect(ch, gsn_weavesense));
    if (paf != NULL)
    {
        send_to_char("You close your mind to the Weave, and soon the flows of Quintessence fade from your sight.\n", ch);
        affect_remove(ch, paf);
        return;
    }

    // Check for the skill to turn it on
    int skill(get_skill(ch, gsn_weavesense));
    if (skill <= 0)
    {
        send_to_char("Huh?\n", ch);
        return;
    }

    // Check for the required mana
    if (ch->mana < skill_table[gsn_weavesense].min_mana)
    {
        send_to_char("You lack the energy to open your mind to the Weave.\n", ch);
        return;
    }

    send_to_char("You open your mind to the Weave, allowing the primal energies of the world to wash over your senses.\n", ch);

    // Turn on the effect
    AFFECT_DATA af = {0};
    af.where     = TO_AFFECTS;
    af.type      = gsn_weavesense;
    af.level     = ch->level;
    af.duration  = -1;
    af.modifier  = skill;
    affect_to_char(ch, &af);
    
    // Handle mana cost and lag
    expend_mana(ch, skill_table[gsn_weavesense].min_mana);
    WAIT_STATE(ch, UMAX(ch->wait, skill_table[gsn_weavesense].beats));
}

bool spell_affinity( int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    OBJ_DATA *obj;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
    int x;
		
    obj = (OBJ_DATA *) vo;

    if (obj == NULL)
    {
        send_to_char("You can't find any such object.\n\r", ch);
        return FALSE;
    }

    if (is_affected(ch, sn))
    {
        send_to_char("You can only place an affinity on one object at a time.\n\r", ch);
        return FALSE;
    }

    if (obj_is_affected(obj, sn))
    {
        send_to_char("That object already has an affinity.\n\r", ch);
        return FALSE;
    }

    af.where     = TO_OBJECT;
    af.type      = sn;
    af.level     = level;
    af.duration  = -1;
    af.location  = 0;
    af.modifier  = x = number_range(1, 15000);
    af.bitvector = ITEM_AFFINITY;
    affect_to_obj(obj, &af);

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = -1;
    af.location  = APPLY_HIDE;
    af.modifier  = x;
    af.bitvector = 0;
    affect_to_char(ch, &af);

    act("$n chants softly, and $p glows brightly for a moment.", ch, obj, NULL, TO_ROOM);
    act("You chant softly, and $p glows brightly for a moment.", ch, obj, NULL, TO_CHAR);

    return TRUE;
}

bool spell_unfettermana(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    // Check for already present
    if (is_affected(ch, sn))
    {
        send_to_char("You already have mana waiting to be unleashed.\n", ch);
        return false;
    }

    // Check for an argument
    if (target_name[0] == '\0')
    {
        send_to_char("You must specify which spell to hold in wait.\n", ch);
        return false;
    }

    // Find the spell
    int storedSN(find_spell(ch, target_name, NULL));
    if (storedSN < 0 
    || skill_table[storedSN].spell_fun == spell_null || skill_table[storedSN].spell_fun == spell_form 
    || skill_table[storedSN].spell_fun == spell_focus || skill_table[storedSN].spell_fun == spell_song)
    {
        send_to_char("You know of no such spell to store.\n", ch);
        return false;
    }

    // Check that the character knows the spell
    int skill(get_skill(ch, storedSN));
    if (skill <= 0 || ch->level < skill_table[storedSN].skill_level[ch->class_num])
    {
        send_to_char("You know of no such spell to store.\n", ch);
        return false;
    }

    // Check that the spell can be cast in battle
    if (skill_table[storedSN].minimum_position > POS_FIGHTING)
    {
        send_to_char("That kind of magic cannot be released in combat.\n", ch);
        return false;
    }

    // Check for mana
    if (ch->mana < skill_table[storedSN].min_mana)
    {
        send_to_char("You are too tired to bind those magics.\n", ch);
        return false;
    }

    // Check that the character casts the spell correctly
    if (number_percent() > skill)
    {
        send_to_char("You lost your concentration while preparing the stored magic.\n", ch);
        expend_mana(ch, skill_table[storedSN].min_mana / 2);
        return true;
    }

    // Add the affect
    AFFECT_DATA af = {0};
    af.where    = TO_AFFECTS;
    af.type     = sn;
    af.level    = level;
    af.modifier = storedSN;
    af.duration = level;
    affect_to_char(ch, &af);

    expend_mana(ch, skill_table[storedSN].min_mana);
    WAIT_STATE(ch, UMAX(ch->wait, skill_table[storedSN].beats));
    send_to_char("You gather up tendrils of energy, weaving them into magics to be unleashed later.\n", ch);
    return true;
}

std::vector<CHAR_DATA*> MembersOfAetherealCommunion(CHAR_DATA * ch, unsigned int & bestModifier)
{
    // Handle a null input
    std::vector<CHAR_DATA*> members;
    if (ch == NULL || ch->in_room == NULL)
        return members;

    bool found(false);
    bestModifier = 0;
    for (CHAR_DATA * gch(ch->in_room->people); gch != NULL; gch = gch->next_in_room)
    {
        // Filter out non-groupmates and NPCs
        if (IS_NPC(gch) || !is_same_group(gch, ch))
            continue;
        
        // Check for avatar
        if (is_an_avatar(gch))
        {
            // If this is the target character, just bail out with an empty list
            if (ch == gch) 
                return std::vector<CHAR_DATA*>();

            // Not the target, so just not counted in the mana drain
            continue;
        }

        // Check whether the effect is up
        members.push_back(gch);
        AFFECT_DATA * paf(get_affect(gch, gsn_aetherealcommunion));
        if (paf != NULL && paf->modifier >= 0)
        {
            found = true;
            bestModifier = UMAX(bestModifier, static_cast<unsigned int>(paf->modifier));
        }
    }

    // Having collected all the groupmates, clear the list back out if none actually had the effect up
    if (!found)
        members.clear();

    return members;
}

bool spell_aetherealcommunion(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    // Check whether already hosting
    AFFECT_DATA * paf(get_affect(ch, sn));
    if (paf != NULL)
    {
        if (paf->modifier < 0)
            send_to_char("You are not yet prepared to host another aethereal communion.\n", ch);
        else
            send_to_char("You are already hosting an aethereal communion.\n", ch);
        return false;
    }

    // Check already-linked members, then let the caster cast the spell
    unsigned int dummy;
    std::vector<CHAR_DATA*> members(MembersOfAetherealCommunion(ch, dummy));

    AFFECT_DATA af = {0};
    af.where    = TO_AFFECTS;
    af.type     = sn;
    af.level    = level;
    af.duration = (level / 5);
    af.modifier = UMAX(0, get_skill(ch, sn));
    affect_to_char(ch, &af);

    if (members.empty())
    {
        std::vector<CHAR_DATA*> membersAfter(MembersOfAetherealCommunion(ch, dummy));
        if (membersAfter.size() > 1)
        {
            // Wasn't already in a linkup, so echo to the whole group
            for (CHAR_DATA * gch(ch->in_room->people); gch != NULL; gch = gch->next_in_room)
            {
                if (is_same_group(gch, ch))
                    send_to_char("A wave of joy washes over you as your spirit links to a greater Whole.\n", gch);
            }
        }
        else
            send_to_char("You prepare to host an aethereal communion.\n", ch);
    }
    else
        send_to_char("You begin to share in the hosting of the aethereal communion.\n", ch);

    return true;
}

bool spell_astralform(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if (!ch->in_room)
        return TRUE;

    if (is_affected(ch, gsn_astralform))
    {
        send_to_char("You are already shifted into the astral plane.\n\r", ch);
        return FALSE;
    }

    send_to_char("You shift slightly into the astral plane.\n\r",ch);
    act("$n grows insubstantial, almost as if $e isn't completely here.",
        ch,NULL,NULL,TO_ROOM);

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = level * 2 / 5;
    af.location  = APPLY_NONE;
    af.modifier  = 0;
    af.bitvector = 0;
    affect_to_char(ch,&af); 

    if (!IS_AFFECTED(ch,AFF_PASS_DOOR))
    {
        af.where     = TO_AFFECTS;
        af.type      = sn;
        af.level     = level;
        af.duration  = level * 2 / 5;
        af.location  = APPLY_NONE;
        af.modifier  = 0;
        af.bitvector = AFF_PASS_DOOR;
        affect_to_char( ch, &af );
    }

    af.location  = APPLY_RESIST_MAGIC;
    af.modifier  = -5;
    affect_to_char (ch, &af); 

    return TRUE;
}


bool spell_astralprojection( int sn, int level,CHAR_DATA *ch,void *vo,int target)
{
    char buf[MAX_STRING_LENGTH];
    CHAR_DATA *spirit;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if ( ch->desc == NULL )
        return FALSE;

    if ( ch->desc->original != NULL )
        return FALSE;

        spirit = create_mobile(get_mob_index(MOB_VNUM_ASTRAL_PROJECTION));
        sprintf(buf, spirit->name, ch->name);
        setName(*spirit, buf);
        sprintf(buf, spirit->short_descr, ch->name);
        free_string(spirit->short_descr);
        spirit->short_descr = str_dup(buf);
        sprintf(buf, spirit->long_descr, ch->name);
        free_string(spirit->long_descr);
        spirit->long_descr = str_dup(buf);
        sprintf(buf, spirit->description, ch->name);
        free_string(spirit->description);
        spirit->description = str_dup(buf);
        spirit->level = ch->level;
        spirit->hit = 100;
        spirit->max_hit = 100;
        spirit->mana = spirit->max_mana = ch->mana;



    act("$n's expression grows vacant, as though $e isn't really here any more.", ch, NULL, NULL, TO_ROOM);
    act("You concentrate, and push your spirit out of your body and lodge yourself on the astral plane.", ch, NULL, NULL, TO_CHAR);

        char_to_room(spirit, ch->in_room);

        af.where     = TO_AFFECTS;
        af.type      = gsn_astralprojection;
        af.level     = level;
        af.duration  = -1;
        af.location  = APPLY_NONE;
        af.modifier  = 0;
        af.bitvector = 0;
        affect_to_char(spirit, &af);

    ch->desc->character = spirit;
    ch->desc->original  = ch;
    spirit->desc        = ch->desc;
    spirit->exp         = ch->exp;
    ch->desc            = NULL;
    /* change communications to match */
    if (ch->prompt != NULL)
        spirit->prompt = str_dup(ch->prompt);
    spirit->comm = ch->comm;
    spirit->lines = ch->lines;
    WAIT_STATE(spirit, UMAX(spirit->wait, 3*PULSE_VIOLENCE));
    WAIT_STATE(ch, UMAX(spirit->wait, 3*PULSE_VIOLENCE));

    send_to_char("You shudder as you realize you are not within your body any more.\n\r", spirit);
    return TRUE;
}


bool spell_aura( int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    OBJ_DATA *obj = (OBJ_DATA *)vo;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if (is_affected(ch, sn))
    {
        send_to_char("You already have added an aura too recently.\n\r", ch);
        return FALSE;
    }

        if (obj->item_type == ITEM_MONEY)
        {
            send_to_char("You can't place an aura on money.\n\r", ch);
            return FALSE;
        }

        if (obj_is_affected(obj, sn))
        {
            send_to_char("That object already has an aura around it.\n\r", ch);
            return FALSE;
        }

        if (obj_is_affected(obj, gsn_soulreaver))
	{
	    act("$p has been bound with fell runes of binding.",
	      ch,obj,NULL,TO_CHAR);
	    return FALSE;
	}

        af.where     = TO_OBJECT;
        af.type      = sn;
        af.level     = level;
        af.duration  = level;
        af.location  = 0;
        af.modifier  = 0;
        af.bitvector = IS_SET(obj->extra_flags[0],ITEM_GLOW) ? 0 : ITEM_GLOW;
        affect_to_obj(obj, &af);

        af.where     = TO_AFFECTS;
        af.type      = sn;
        af.level     = level;
        af.duration  = level;
        af.location  = APPLY_MANA;
        af.modifier  = -1 * level / 2;
        af.bitvector = 0;
        affect_to_char(ch, &af);

        act("$n whispers, and $p glows with a soft warm light.", ch, obj, NULL, TO_ROOM);
        act("You whisper, and $p glows with a soft warm light.", ch, obj, NULL, TO_CHAR);

    return TRUE;
}

bool spell_avatar( int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if (IS_NAFFECTED(ch, AFF_AVATAR))
    {
        send_to_char("You can't assume the form of an avatar again so soon.\n\r", ch);
        return FALSE;
    }

        act("$n rises from the ground, trembling, as $s body explodes in brilliant light!", ch, NULL, NULL, TO_ROOM);
        act("You rise from the ground, trembling uncontrollably.", ch, NULL, NULL, TO_CHAR);
        act("Your body explodes in pain and power as you transform!", ch, NULL, NULL, TO_CHAR);

        af.where     = TO_AFFECTS;
        af.type      = sn;
        af.level     = level;
        af.duration  = -1;
        af.location  = APPLY_HIT;
        af.modifier  = ch->max_hit;
        af.bitvector = 0;
        affect_to_char(ch, &af);

        af.location  = APPLY_HITROLL;
        af.modifier  = level;
        affect_to_char(ch, &af);

        af.location  = APPLY_DAMROLL;
        af.modifier  = level;
        affect_to_char(ch, &af);

        ch->hit = ch->max_hit;

        affect_strip(ch, gsn_sanctuary);

        if (ch->long_descr && ch->long_descr != &str_empty[0])
          free_string(ch->long_descr);
        ch->long_descr = str_dup("A shifting being of pure spirit floats through the area, bathing it in an eerie blue light.\n\r");

    return TRUE;
}

bool spell_avengingseraph( int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    if (is_affected(ch, sn))
    {
        send_to_char("You are already protected by a guardian angel.\n\r", ch);
        return false;
    }

    AFFECT_DATA af = {0};
    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = level;
    affect_to_char(ch, &af);

    act("A translucent pair of wings form behind $n briefly, then dissolve into nothing.", ch, NULL, NULL, TO_ROOM);
    act("A translucent pair of wings form behind you briefly, then dissolve into nothing.", ch, NULL, NULL, TO_CHAR);
    return true;
}

bool spell_awaken( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;

    if (ch->position > POS_SLEEPING)
    {
    	if (victim == ch)
            act("$n enters a trance for a moment.", ch, NULL, NULL, TO_ROOM);
        else
        {
    	    act("$n places $s hand upon $N's head, and concentrates a moment.", ch, NULL, victim, TO_ROOM);
            act("You place your hand upon $N's head.", ch, NULL, victim, TO_CHAR);
            act("Your mind clears as $N places $S hand upon your forehead.", victim, NULL, ch, TO_CHAR);
        }
    }

    if (check_dispel(level,victim,gsn_sleep))
    {
        act("You feel more awake.", victim,NULL,NULL,TO_CHAR);
        act("$n looks more awake.",victim,NULL,NULL,TO_ROOM);
    }

    if (is_affected(victim, gsn_waylay) || is_affected(victim, gsn_pummel)) 
    {
	send_to_char("Your head feels better.\n\r", victim);
	act("$n looks more awake.", victim, NULL, NULL, TO_ROOM);
	affect_strip(victim, gsn_waylay);
	affect_strip(victim, gsn_pummel);
    }

    if (is_affected(victim,gsn_garrote) && IS_AFFECTED(victim, AFF_SLEEP))
    {
	send_to_char("Your neck feels better.\n\r", victim);
	act("$n looks more awake.", victim, NULL, NULL, TO_ROOM);
	affect_strip(victim,gsn_garrote);
    }
    
    return TRUE;
}

bool spell_bless( int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim;
    OBJ_DATA *obj;
    AFFECT_DATA af = {0};

    /* deal with the object case first */
    if (target == TARGET_OBJ)
    {
        obj = (OBJ_DATA *) vo;
        if (IS_OBJ_STAT(obj,ITEM_BLESS))
        {
            act("$p is already blessed.",ch,obj,NULL,TO_CHAR);
            return FALSE;
        }

        if (IS_OBJ_STAT(obj,ITEM_EVIL))
        {
            AFFECT_DATA *paf;
            if (obj_is_affected(obj, gsn_soulreaver))
            {
                act("$p has been consecrated to the Gods of Darkness.", ch, obj, NULL, TO_CHAR);
                return TRUE;
            }
            if (obj_is_affected(obj, gsn_baneblade))
            {
                act("$p is far too unholy for such magics.", ch, obj, NULL, TO_CHAR);
                return true;
            }
            paf = affect_find(obj->affected,gsn_curse);
            if (!saves_dispel(level,paf != NULL ? paf->level : obj->level,0))
            {
                if (paf != NULL)
                    affect_remove_obj(obj,paf);
                act("$p glows a pale blue.",ch,obj,NULL,TO_ALL);
                REMOVE_BIT(obj->extra_flags[0],ITEM_EVIL);
                return TRUE;
            }
            else
            {
                act("The evil of $p is too powerful for you to overcome.", ch,obj,NULL,TO_CHAR);
                return TRUE;
            }
        }

        SET_BIT(obj->extra_flags[0], ITEM_BLESS);
        act("$p glows with a holy aura.",ch,obj,NULL,TO_ALL);
        return TRUE;
    }

    /* character target */
    victim = (CHAR_DATA *) vo;

    if (is_affected(victim, sn))
    {
        if (victim == ch) send_to_char("You are already blessed.\n\r",ch);
        else act("$N already has divine favor.",ch,NULL,victim,TO_CHAR);
        return FALSE;
    }
   
    int hitrollMod(UMAX(1, level / 8));
    int saveMod(UMIN(-1, -level / 8));
    if (perform_litany_skill_check(ch, NULL, Litany_Benediction, 1))
    {
        hitrollMod += 2;
        saveMod -= 2;
    }
    
    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = (6 + level) * (check_durablemagics(*ch) ? 2 : 1);
    af.location  = APPLY_HITROLL;
    af.modifier  = hitrollMod;
    af.bitvector = 0;
    affect_to_char( victim, &af );

    af.location  = APPLY_SAVING_SPELL;
    af.modifier  = saveMod;
    affect_to_char( victim, &af );
    send_to_char( "You feel righteous.\n\r", victim );
    if (ch != victim)
        act("You imbue $N with divine favor.",ch,NULL,victim,TO_CHAR);

    return TRUE;
}

bool spell_calm( int sn, int level, CHAR_DATA *ch, void *vo,int target)
{
    CHAR_DATA *vch;
    int mlevel = 0;
    int count = 0;
    int high_level = 0;
    int chance;

    /* get sum of all mobile levels in the room */
    for (vch = ch->in_room->people; vch != NULL; vch = vch->next_in_room)
    {
        if (vch->position == POS_FIGHTING)
        {
            count++;
            if (IS_NPC(vch))
              mlevel += vch->level;
            else
              mlevel += vch->level/2;
            high_level = UMAX(high_level,vch->level);
        }
    }

    /* compute chance of stopping combat */
    chance = 4 * level - high_level + 2 * count;

    if (IS_IMMORTAL(ch)) /* always works */
      mlevel = 0;

    if (number_range(0, chance) >= mlevel)  /* hard to stop large fights */
    {
        for (vch = ch->in_room->people; vch != NULL; vch = vch->next_in_room)
        {
            if (IS_NPC(vch) && (IS_SET(vch->imm_flags,RESIST_MAGIC) ||
                                IS_SET(vch->act,ACT_UNDEAD)))
              return TRUE;

            if (IS_AFFECTED(vch,AFF_CALM) || IS_AFFECTED(vch,AFF_BERSERK)
            ||  is_affected(vch,skill_lookup("frenzy")))
                return TRUE;

            send_to_char("A wave of calm passes over you.\n\r",vch);
            if (vch->fighting || vch->position == POS_FIGHTING) {
                stop_fighting(vch);
                vch->tracking = NULL;
            }
        }
    }
    return TRUE;
}


bool spell_clarity( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo, *cch;

    if (!IS_NPC(ch) && !IS_AFFECTED(ch, AFF_WIZI))
    {
        if (victim == ch)
        {
            act("$n concentrates momentarily.", ch, NULL, NULL, TO_ROOM);
            act("You concentrate momentarily.", ch, NULL, NULL, TO_CHAR);
        }
        else
        { 
            act("$n lifts a hand towards $N, who glows in a soft light!",
		ch, NULL, victim, TO_NOTVICT);
            act("You lift a hand towards $N, who glows in a soft light!",
		ch, NULL, victim, TO_CHAR);
            act("$n lifts a hand towards you, and you glow in a soft light!",
		ch, NULL, victim, TO_VICT);
        }
    }

    if (check_dispel(level+3,victim,gsn_delusions))
    {
        act("$n is no longer deluded.",victim,NULL,NULL,TO_NOTVICT);
        act("You are no longer deluded.", victim, NULL, NULL, TO_CHAR);
    }
    if (check_dispel(level+3,victim,gsn_obfuscation))
    {
        act("$n realizes someone was obfuscated from $s vision.",victim,NULL,NULL,TO_NOTVICT);
        act("You realize someone had been obfuscated from your vision.", victim, NULL, NULL, TO_CHAR);
    }
    if (check_dispel(level+3,victim,gsn_nightfears))
    {
        act("$n's nightmares subside.",victim,NULL,NULL,TO_NOTVICT);
        act("Your nightmares subside.", victim, NULL, NULL, TO_CHAR);
    }
    if (check_dispel(level+3,victim,gsn_powerwordfear))
    {
        act("$n's unfounded fear is banished.",victim,NULL,NULL,TO_NOTVICT);
        act("Your unfounded fear is banished.", victim, NULL, NULL, TO_CHAR);
    }
    if (check_dispel(level+3,victim,gsn_visions))
    {
        act("$n sees the world as it is again.",victim,NULL,NULL,TO_NOTVICT);
        act("The visions of damnation chasing you disappear suddenly.", victim, NULL, NULL, TO_CHAR);
    }

    if (cch=cloak_remove(victim))
    {
	send_to_char("You feel your cloak forced away.\n\r",cch);
	send_to_char("You realize that someone was obscured from your sight.\n\r",victim);
    }

    return TRUE;
}

static bool check_can_consecrate(CHAR_DATA * ch, OBJ_DATA * obj)
{
    if (obj_is_affected(obj, gsn_consecrate) || (obj->value[3] == attack_lookup("divine")))
    {
        send_to_char("That weapon has already been consecrated.\n\r", ch);
        return false;
    }

    if (IS_SET(obj->extra_flags[0], ITEM_EVIL) || IS_SET(obj->extra_flags[0], ITEM_ANTI_GOOD) || IS_SET(obj->extra_flags[0], ITEM_DARK))
    {
        act("The dark aura surrounding $p prevent your attempts to consecrate it.", ch, obj, NULL, TO_CHAR);
        return FALSE;
    }

    if (obj->value[3] == DAM_NEGATIVE || obj->value[3] == DAM_DEFILEMENT)
    {
        send_to_char("You cannot consecrate a weapon of the void.\n\r", ch);
        return FALSE;
    }

    return true;
}

bool spell_consecrate( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    OBJ_DATA *vObj = (OBJ_DATA *) vo;
    AFFECT_DATA af = {0};

    if (!(vObj->item_type == ITEM_WEAPON || vObj->item_type == ITEM_ARROW))
    {
        send_to_char("Only weapons may be consecrated.\n\r", ch);
        return FALSE;
    }

    if (!check_can_consecrate(ch, vObj))
        return false;

    af.where     = TO_OBJECT;
    af.type      = sn;
    af.level     = level;
    af.duration  = level/3;
    affect_to_obj(vObj, &af);

    act("$p shines briefly with a pure white aura.", ch, vObj, NULL, TO_CHAR);
    act("$n chants, and $p shines with a pure white aura.", ch, vObj, NULL, TO_ROOM);

    return TRUE;
}


bool spell_continual_light(int sn,int level,CHAR_DATA *ch,void *vo,int target)
{
    OBJ_DATA *light;
    AFFECT_DATA *paf;
    if (target_name[0] != '\0')  /* do a glow on some object */
    {
        light = get_obj_carry(ch,target_name,ch);

        if (light == NULL)
        {
            send_to_char("You don't see that here.\n\r",ch);
            return TRUE;
        }

        if (IS_OBJ_STAT(light,ITEM_GLOW))
        {
            act("$p is already glowing.",ch,light,NULL,TO_CHAR);
            return TRUE;
        }
	if ((paf = affect_find(light->affected,gsn_dim)) != NULL)
	{
            if (!saves_dispel(level,paf->level,paf->duration))
	    {
		object_affect_strip(light,gsn_dim);
		act("You remove the dimness from $p.",ch,light,NULL,TO_CHAR);
	    }
	    else
	        act("You try to remove the dimness from $p, but fail.",ch,light,NULL,TO_CHAR);
	    return TRUE;
	}
        SET_BIT(light->extra_flags[0],ITEM_GLOW);
        act("$p glows with a white light.",ch,light,NULL,TO_ALL);
        return TRUE;
    }

    if (room_is_affected(ch->in_room,gsn_dim))
    {
	if ((paf = affect_find(ch->in_room->affected,gsn_dim)) != NULL)
	    if (!saves_dispel(level,paf->level,paf->duration))
	    {
		room_affect_strip(ch->in_room,gsn_dim);
		act("The ambient light returns to normal.",ch,NULL,NULL,TO_CHAR);
	    }
	    else
	        act("The area remains dim.",ch,NULL,NULL,TO_CHAR);
	return TRUE;
    }
    AFFECT_DATA af;
    af.valid = TRUE;
    af.point = NULL;
    af.modifier = 0;
    af.duration = 12;
    af.bitvector = 0;
    af.level = level;
    af.type = sn;
    af.where = TO_ROOM_AFF;
    affect_to_room(ch->in_room, &af);
    act("$n chants, and the ambient light is amplified.",ch,NULL,NULL,TO_ROOM);
    act("The ambient light takes on a soft, amplified glow.",ch,NULL,NULL,TO_CHAR);
    return TRUE;
}

bool spell_createfoci( int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    OBJ_DATA *obj = (OBJ_DATA *) vo;

    if (IS_NPC(ch))
    {
	send_to_char("Mobiles may not create foci.\n\r", ch);
	return FALSE;
    }

    if (obj_is_affected(obj, sn))
    {
        send_to_char("That item is already a focus.\n\r", ch);
        return FALSE;
    }

    if (is_affected(ch, gsn_createfoci))
    {
	send_to_char("You do not feel ready to create another focus.\n\r", ch);
	return FALSE;
    }

    if (get_foci(ch, TRUE) != NULL)
    {
        send_to_char("Your powers are already focused upon another item in this world.\n\r", ch);
        return FALSE;
    }

    // Get some quintessence
    OBJ_DATA * quintessence(require_quintessence(ch));
    if (quintessence == NULL)
        return false;

    AFFECT_DATA af = {0};
    af.where     = TO_OBJECT;
    af.type      = sn;
    af.level     = level;
    af.duration  = -1;
    af.location  = APPLY_HIDE;
    af.modifier  = ch->id;
    af.bitvector = ITEM_NODESTROY;
    affect_to_obj(obj, &af);

    af.where	 = TO_CHAR;
    af.duration  = 4800;
    af.location  = APPLY_HIDE;
    af.modifier  = 0;
    af.bitvector = 0;
    affect_to_char(ch, &af);

    if (!IS_SET(obj->wear_flags, ITEM_NO_SAC))
    	SET_BIT(obj->wear_flags, ITEM_NO_SAC);

    act("You feel more in touch with $p.", ch, obj, NULL, TO_CHAR);
    
    // Consume the quintessence    
    consume_quintessence(ch, quintessence);
    return TRUE;
}

bool spell_dreamspeak(int sn,int level,CHAR_DATA *ch,void *vo, int target)
{
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if (is_affected(ch, sn))
    {
        send_to_char("You are already able to speak in dreams.\n\r", ch);
        return FALSE;
    }

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = 24;
    af.location  = 0;
    af.modifier  = 0;
    af.bitvector = 0;
    affect_to_char( ch, &af );
    send_to_char("You feel in touch with the dreamworld.\n\r", ch);
    return TRUE;
}


bool spell_focusmind(int sn,int level,CHAR_DATA *ch,void *vo, int target)
{
    AFFECT_DATA af = {0};

    if (is_affected(ch, sn))
    {
        send_to_char("You cannot focus your mind again yet.\n", ch);
        return false;
    }
    
    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = level / 10;
    af.modifier  = UMAX(50, get_skill(ch, gsn_focusmind));
    affect_to_char(ch, &af);
    send_to_char("You feel able to focus your thoughts sharply.\n", ch);
    return true;
}


bool spell_exorcism( int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    OBJ_DATA *vObj;
    CHAR_DATA *victim;
    AFFECT_DATA af;
    af.valid = TRUE;
    af.point = NULL;

    if (target == TARGET_OBJ)
    {
        vObj = (OBJ_DATA *) vo;

        act("You begin a holy chant, preparing to cleanse $p.", ch, vObj, NULL, TO_CHAR);
        act("$n begins a holy chant, preparing to cleanse $p.", ch, vObj, NULL, TO_ROOM);
        act("As you complete the chant, you thrust $p towards the heavens, shouting a word of power!", ch, vObj, NULL, TO_CHAR);
        act("As $n completes the chant, $e thrusts $p towards the heavens, shouting a word of power!", ch, vObj, NULL, TO_ROOM);
        act("$p is surrounded by a brilliant white glow.", ch, vObj, NULL, TO_CHAR);
        act("$p is surrounded by a brilliant white glow.", ch, vObj, NULL, TO_ROOM);
        activate_demon_bind(ch, vObj);
        object_affect_strip(vObj, gsn_demon_bind);
        object_affect_strip(vObj, gsn_jawsofidcizon);
        object_affect_strip(vObj, gsn_caressofpricina);
        object_affect_strip(vObj, gsn_blade_of_vershak);
        object_affect_strip(vObj, gsn_hungerofgrmlarloth);
        object_affect_strip(vObj, gsn_reaver_bind);
        object_affect_strip(vObj, gsn_soulreaver);
        object_affect_strip(vObj, gsn_mireofoame);
        return TRUE;
    }

    victim = (CHAR_DATA *) vo;

    if (victim == ch)
    {
        send_to_char("You cannot exorcise yourself.\n\r",ch);
        return FALSE;
    }
   
    if (IS_OAFFECTED(victim,AFF_DEMONPOS))
    {
        if ( saves_spell( level, ch, victim, DAM_HOLY ) )         
        { 
            send_to_char("The demon proves too powerful for you to exorcise.\n\r",ch);
            return TRUE; 
        }
        send_to_char("You make a circular gesture with your hand.\n\r",ch);
        act("You point at $N.",ch,NULL,victim,TO_CHAR);
        act("You cry out, {c'Alam Alam Laneth Rystaia $N!'{x",ch,NULL,victim,TO_CHAR);
        act("$n makes a circular gesture with $s hand.",ch,NULL,victim,TO_NOTVICT);
        act("$n points at $N.",ch,NULL,victim,TO_NOTVICT);
        act("$n cries out, {c'Alam Alam Laneth Rystaia $N!'{x",ch,NULL,victim,TO_NOTVICT);

        act("$n makes a circular gesture with $s hand.",ch,NULL,victim,TO_VICT);
        act("$n points at you.",ch,NULL,victim,TO_VICT);
        act("$n cries out, {c'Alam Alam Laneth Rystaia $N!'{x",ch,NULL,victim,TO_VICT);

        act("As you complete the holy chant, tendrils of white light swirl about $N!",ch,NULL,victim,TO_CHAR);
        act("$N roars, in the throes of some unspeakable agony!",ch,NULL,victim,TO_CHAR);
        act("$N collapses to $S knees, as a swirl of black fog rushes out of $S mouth.",ch,NULL,victim,TO_CHAR);
        act("As $n cries forth the holy chant, tendrils of white light swirl about you!",ch,NULL,victim,TO_VICT);
        act("Deep within, the demon screams in {rUNSPEAKABLE{x agony!",ch,NULL,victim,TO_VICT);
        act("With a roar, the demon rushes out of your body, leaving you spent.",ch,NULL,victim,TO_VICT); 
        act("As $n cries forth the holy chant, tendrils of white light swirl about $N!",ch,NULL,victim,TO_NOTVICT);
        act("$N roars, in the throes of some unspeakable agony!",ch,NULL,victim,TO_NOTVICT);
        act("$N collapses to $S knees, as a swirl black fog rushes out of $S mouth.",ch,NULL,victim,TO_NOTVICT);

        affect_strip(victim,gsn_demonpos);
        victim->hit = victim->hit / 2 + 100 + victim->level;
        victim->mana /= 2;
        switch_position(victim, POS_RESTING);

        af.where     = TO_AFFECTS;
        af.type      = gsn_demonpos;
        af.level     = victim->level;
        af.duration  = 15;
        af.location  = APPLY_HIT;
        af.modifier  = -100 - victim->level;
        af.bitvector = 0;
        affect_to_char(victim, &af);         
        return TRUE; 

    } 

    else if ((victim->demontrack != NULL) && IS_NPC(victim))
    
    {
        if ( saves_spell( level+7, ch, victim, DAM_HOLY ) )
        {
            send_to_char("The demon proves too powerful to exorcise.\n\r",ch);
            return TRUE;
        }
        send_to_char("You make a circular gesture with your hand.\n\r",ch);
        act("You point at $N.",ch,NULL,victim,TO_CHAR);
        act("You cry out, '{cNalin Nalin Mansa Rystaia $N!'{x",ch,NULL,victim,TO_CHAR);
        act("$n makes a circular gesture with $s hand.",ch,NULL,victim,TO_NOTVICT);
        act("$n points at $N.",ch,NULL,victim,TO_NOTVICT);
        act("$n cries out, {c'Nalin Nalin Mansa Rystaia $N!'{x",ch,NULL,victim,TO_NOTVICT);
        act("As you complete the holy chant, a pale glow flickers about $N's flesh.",ch,NULL,victim,TO_CHAR);
        act("$N begins shrieking, as the light grows to cover $S entire body!",ch,NULL,victim,TO_CHAR);
        act("A sickening thud sounds, and the demon implodes!",ch,NULL,victim,TO_CHAR);
        act("There is a flash of light, and no trace of the demon remains.",ch,NULL,victim,TO_CHAR);
        act("As $n completes the holy chant, a pale glow flickers about $N's flesh.",ch,NULL,victim,TO_NOTVICT);
        act("$N begins shrieking, as the light grows to cover $S entire body!",ch,NULL,victim,TO_NOTVICT);
        act("A sickening thud sounds, and the demon implodes!",ch,NULL,victim,TO_NOTVICT);
        act("There is a flash of light, and no trace of the demon remains.",ch,NULL,victim,TO_NOTVICT);
        extract_char(victim, TRUE);        
        return TRUE;
    }           

    else if ( victim->desc != NULL && victim->desc->original != NULL 
         &&   is_affected(victim,gsn_possession))  
    {
		        
        if ( saves_spell( level, ch, victim, DAM_HOLY ) )
        {
            send_to_char("The scholar proves too powerful to exorcise.\n\r",ch);
            return TRUE;
        } 
        act("You make a circular gesture with your hand.",ch,NULL,victim,TO_CHAR);
        act("You point at $N.",ch,NULL,victim,TO_CHAR);
        act("You cry out, {c'Dalan Dalan Kellis Rystaia $N!'{x",ch,NULL,victim->desc->original, TO_CHAR);

        act("$n makes a circular gesture with $s hand.",ch,NULL,victim,TO_NOTVICT);
        act("$n points at $N.",ch,NULL,victim,TO_NOTVICT);
        act("$n cries out, {c'Dalan Dalan Kellis Rystaia $N!'{x",ch,NULL,victim->desc->original,TO_NOTVICT);
        
        act("$n makes a circular gesture with $s hand.",ch,NULL,victim,TO_VICT);
        act("$n points at you.",ch,NULL,victim,TO_VICT);
        act("$n cries out, {c'Dalan Dalan Kellis Rystaia $N!{x",ch,NULL,victim->desc->original,TO_VICT);

	act("As you complete the holy chant, a silver light flickers around $N.",ch,NULL,victim,TO_CHAR);
	act("A dark, translucent shape is slowly drawn out of $N, and fades away at contact with the silver light.",ch,NULL,victim,TO_CHAR);
	act("$N's eyes lose their slightly glazed look.",ch,NULL,victim,TO_CHAR);
	
	act("As $n completes the holy chant, a silver light flickers about you.",ch,NULL,victim,TO_VICT);
	act("Your soul recoils at the holy light, and is driven away from your host!",ch,NULL,victim,TO_VICT);
	act("With a blast, you are driven back to your mortal body.",ch,NULL,victim,TO_VICT);
	
	act("As $n completes the holy chant, a silver light flickers about $N.",ch,NULL,victim,TO_NOTVICT);
	act("A dark, translucent shape is slowly drawn out of $N, and fades away at contact with the silver light.",ch,NULL,victim,TO_NOTVICT);
	act("$N looks bewildered",ch,NULL,victim,TO_NOTVICT);
        do_relinquish(victim,"notext");
        return TRUE;
    }

    else if (is_affected(victim, gsn_devouringspirit))
    {
	if (saves_spell(level, ch, victim, DAM_MENTAL))
	{
	    act("The hunger remains in $S eyes.", ch, NULL, victim, TO_CHAR);
	    return TRUE;
	}

	act("You make a circular gesture with your hand.", ch, NULL, victim, TO_CHAR);
	act("You point at $N.", ch, NULL, victim, TO_CHAR);

	act("$n makes a circular gesture with $s hand.", ch, NULL, NULL, TO_ROOM);
	
	act("$n points at $N.", ch, NULL, victim, TO_NOTVICT);
	act("$n points at you.", ch, NULL, victim, TO_VICT);

	act("A dark presence writhes up out of $n's body as the devouring spirit is exorcized.", victim, NULL, NULL, TO_ROOM);
        act("A dark presence writhes up out of your body, and your devouring hunger fades.", victim, NULL, NULL, TO_CHAR);

	affect_strip(victim, gsn_devouringspirit);
	return TRUE;
    }

    // Look for hauntings
    std::vector<AFFECT_DATA *> hauntings(findHauntings(victim));
    if (hauntings.empty())
    {
        send_to_char("You can find nothing to exorcise in your target.\n\r",ch);
        return false;
    }

    act("You make a circular gesture with one hand, murmuring a word of exorcism as you point at $N.", ch, NULL, victim, TO_CHAR);
    act("$n makes a circular gesture with one hand, murmuring a word of exorcism as $e points at you.", ch, NULL, victim, TO_VICT);
    act("$n makes a circular gesture with one hand, murmuring a word of exorcism as $e points at $N.", ch, NULL, victim, TO_NOTVICT);
    act("The spirits haunting $M dissipate, exorcised by your power.", ch, NULL, victim, TO_CHAR);
    act("Your back unclenches as the spirits haunting you dissipate, exorcised by $n's power.", ch, NULL, victim, TO_VICT);

    for (size_t i(0); i < hauntings.size(); ++i)
        affect_remove(victim, hauntings[i]);
	
    return TRUE;
}

bool spell_lifebolt( int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    int dam;
    char buf[MAX_STRING_LENGTH];
// brazen: Ticket #364: life bolt misgrammar
    act("$n channels $s life force into a blast of energy, striking $N!", ch, NULL, victim, TO_NOTVICT);
    act("You channel your life force into a blast of energy, striking $N!", ch, NULL, victim, TO_CHAR);
    act("$n channels $s life force into a blast of energy, striking you!", ch, NULL, victim, TO_VICT);

    dam = dice( level, 5 ) + 50;
    ch->hit -= (dam / 9);
    
    // Check for litany of mortification; if not present, check for saves
    if (perform_litany_skill_check(ch, victim, Litany_Mortification, 5))
        act("You mortify $N with your magic, bypassing $S defenses!", ch, NULL, victim, TO_CHAR);
    else if (saves_spell(level, ch, victim, DAM_ENERGY))
        dam = dam / 2;

    // Handle quintessence rush boost
    if (is_quintessence_rushing(ch))
        dam = (dam * 3) / 2;

    damage_old( ch, victim, dam, sn,DAM_ENERGY,TRUE);
    update_pos(ch);
    if (ch->position == POS_DEAD)
    {
        act("$n trembles, having expired too much of $s precious life force.", ch, NULL, NULL, TO_ROOM);
        send_to_char("You tremble, having expired too much of your precious life force.\n\r", ch);

        act("$n collapses to the ground, an empty shell.", ch, NULL, NULL, TO_ROOM);
        send_to_char("Spent of your life force, you collapse to the ground, an empty shell./n/r", ch);
	sprintf(buf,"%s lifebolted %s too much in %s [%i].\n\r",ch->name,victim->name,ch->in_room->name, ch->in_room->vnum);
	log_string(buf);
        raw_kill(ch);
        return TRUE;
    }
    return TRUE;
}

bool spell_lightspear( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    OBJ_DATA *lightspear;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if (is_affected(ch, sn))
    {
        send_to_char("You don't feel ready to create another spear of light yet.\n\r",ch);
        return FALSE;
    }

    if ((lightspear = create_object(get_obj_index(OBJ_VNUM_LIGHT_WEAPON), level)) == NULL)
    {
        bug("Spell: spear of light.  Cannot load spear object.", 0);
        send_to_char("A problem has occured. Please contact the gods.\n\r", ch);
        return FALSE;
    }

    lightspear->level    = level;
    lightspear->timer    = level * 20;
    lightspear->value[0] = 3;
    lightspear->value[1] = 3 + level/6;
    lightspear->value[2] = 3;
    lightspear->weight   = 120;

    setName(*lightspear, "spear light");
    free_string(lightspear->short_descr);
    lightspear->short_descr = str_dup("a spear of light");
    free_string(lightspear->description);
    lightspear->description = str_dup("A spear of light lies here.");

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = 25;
    af.location  = 0;
    af.modifier  = 0;
    af.bitvector = 0;
    affect_to_char(ch, &af);

    send_to_char("You raise your hand towards the sky, and a blazing shaft of light appears within your fist, coalescing into a shimmering spear.\n\r",ch);
    act("$n raises $s hand towards the sky, and a blazing shaft of light appears within $s fist, coalescing into a shimmering spear.", ch, NULL, NULL, TO_ROOM);

    obj_to_char(lightspear, ch);

    return TRUE;
}

bool spell_lightsword( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    OBJ_DATA *lightsword;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if (is_affected(ch, sn))
    {
        send_to_char("You don't feel ready to create another blade of light yet.\n\r",ch);
        return FALSE;
    }

    if ((lightsword = create_object(get_obj_index(OBJ_VNUM_LIGHT_WEAPON), 0)) == NULL)
    {
        bug("Spell: blade of light.  Cannot load sword object.", 0);
        send_to_char("A problem has occurred. Please contact the gods\n\r", ch);
        return FALSE;
    }

    lightsword->level    = level;
    lightsword->timer    = level * 20;
//    lightsword->value[0] = 1;            weapon type should be set up by the actual object in oedit
    lightsword->value[1] = 6 + level/8;
    lightsword->value[2] = 3;
//    lightsword->value[3] = 44;           weapon damage type should be set by the actual object in oedit

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = 25;
    af.location  = 0;
    af.modifier  = 0;
    af.bitvector = 0;
    affect_to_char(ch, &af);

    send_to_char("Dazzling splinters of light appear and stretch across your upturned palms, coalescing into a shining blade!\n\r", ch);
    act("Dazzling splinters of light appear and stretch across $n's upturned palms, coalescing into a shining blade!", ch, NULL, NULL, TO_ROOM);

    obj_to_char(lightsword, ch);

    return TRUE;
}


bool spell_manaconduit(int sn,int level,CHAR_DATA *ch,void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *)vo;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
    int x;

    if (ch == victim)
    {
        send_to_char("You can't link to yourself!\n\r", ch);
        return FALSE;
    }

    if (is_affected(ch, sn))
    {
        send_to_char("You are already tapped by a mana conduit.\n\r", ch);
        return FALSE;
    }

    if (is_affected(victim, sn))
    {
        act("$N is already tapped by a mana conduit.", ch, NULL, victim, TO_CHAR);
        return FALSE;
    }

    if (!is_same_group(ch, victim))
    {
        send_to_char("You must be in a group to tap someone with a mana conduit.\n\r", ch);
        return FALSE;
    }

    if (IS_NPC(victim))
    {
	send_to_char("You cannot seem to tap their power.\n\r", ch);
	return FALSE;
    }

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = level/7;
    af.location  = APPLY_HIDE;
    af.modifier  = x = number_range(1, 10000);
    af.bitvector = 0;
    affect_to_char( ch, &af );
    affect_to_char( victim, &af);

    act("You link yourself to $N with a mana flow, and feel energy coursing along the link.", ch, NULL, victim, TO_CHAR);
    act("You feel $n link to you with a mana flow, and feel energy coursing along the link.", ch, NULL, victim, TO_VICT);
    act("You see a small flow of energy stretch from $n to $N, then fade.", ch,
NULL, victim, TO_NOTVICT);
    return TRUE;
}


bool spell_thanatopsis( int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    if ( is_affected( ch, sn ) )
    {
        send_to_char("You already hear the cries of death.\n\r", ch);
        return FALSE;
    }

    AFFECT_DATA af = {0};
    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = level * (check_durablemagics(*ch) ? 4 : 2);
    affect_to_char( ch, &af );

    act("A calm settles over you for a moment, and you feel more in tune with the world.", ch, NULL, NULL, TO_CHAR);

    return TRUE;
}

// Unseen Servant: c0d3d by Khajran

bool spell_unseenservant(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    OBJ_DATA *item;
    CHAR_DATA *victim; 
    char arg[MAX_STRING_LENGTH];
    bool failed = FALSE;
    
    target_name = one_argument(target_name, arg);
    if (arg[0] == '\0')
    {
        send_to_char("What object do you wish to focus upon?\n\r", ch );
         return FALSE;
    }    
    item = get_obj_carry( ch, arg, ch );
    if(item == NULL) {
       send_to_char( "You are not carrying that.\n\r", ch );
       return FALSE;
    }    

    target_name = one_argument(target_name, arg);
    
    if(arg[0] == '\0' || !(victim = get_char_world(ch, arg)))
    {
       send_to_char("You do not sense that person in the world.\n\r", ch);
       return FALSE;   
    }

    if(  IS_SET(ch->in_room->room_flags, ROOM_NOGATE)
    ||   IS_SET(ch->in_room->room_flags, ROOM_NO_RECALL)
    ||   IS_SET(ch->in_room->room_flags, ROOM_NOMAGIC)
    ||   IS_SET(ch->in_room->room_flags, ROOM_SAFE)
    ||   IS_SET(ch->in_room->room_flags, ROOM_PRIVATE)
    ||   IS_SET(ch->in_room->room_flags, ROOM_SOLITARY)
    ||   IS_SET(ch->in_room->room_flags, ROOM_GODS_ONLY)
    ||   room_is_affected(ch->in_room, gsn_desecration)
    ||   area_is_affected(ch->in_room->area, gsn_suppress) )
    {
       send_to_char("Your magic can't call a servant here.\n\r", ch);
       return FALSE; 
    }
    
    if(victim == ch)
    {
       send_to_char("You can't use the servant to give things to yourself.\n\r", ch);
       return FALSE; 
    }
    
    if (!can_drop_obj(ch, item))
    {
        send_to_char("You can't let go of it.\n\r", ch);
        return FALSE;
    }   
    
    act("You hold out $p, watching it vanish as an unseen form claims it.", ch, item, NULL, TO_CHAR);
    act("$n holds out $p, and it slowly vanishes from sight.", ch, item, NULL, TO_ROOM); 


    if (IS_NPC(victim)
    ||  victim->level > LEVEL_HERO
    ||  victim->in_room == NULL
    ||  IS_SET(victim->in_room->room_flags, ROOM_NOGATE)
    ||  IS_SET(victim->in_room->room_flags, ROOM_NO_RECALL) 
    ||  IS_SET(victim->in_room->room_flags, ROOM_NOMAGIC)
    ||  IS_SET(victim->in_room->room_flags, ROOM_SAFE)
    ||  IS_SET(victim->in_room->room_flags, ROOM_PRIVATE)
    ||  IS_SET(victim->in_room->room_flags, ROOM_SOLITARY)
    ||  IS_SET(victim->in_room->room_flags, ROOM_GODS_ONLY)
    ||  room_is_affected(victim->in_room, gsn_desecration)
    ||  area_is_affected(victim->in_room->area, gsn_suppress)
    ||  IS_PAFFECTED(victim, AFF_VOIDWALK))
    {
       failed = TRUE;
    }

    // Check for saves unless nosummon is off; runes of spirit mean automatic saves
    if (IS_SET(victim->act, PLR_NOSUMMON) && (obj_is_affected(item, gsn_runeofspirit) || saves_spell(level, ch, victim, DAM_OTHER)))
    {
        failed = TRUE;
        send_to_char("You feel an unseen presence draw near, but push it away it with a force of will.\n", victim);
    }

    if (failed) 
    {
        act("There is a brief flash of light, and the feel of some unknown presence is apparent.", ch, item, NULL, TO_ROOM);           
        act("There is a brief flash of light, and the feel of some unknown presence is apparent.", ch, item, NULL, TO_CHAR);
        act("Unable to deliver the item, the unseen servant returns $p to you.",ch, item, NULL, TO_CHAR);
        act("Unable to deliver the item, the unseen servant returns $p to $n.", ch, item, NULL, TO_ROOM);
        return TRUE;  
    }

    // Check for noaccept to determine whether to deliver the item to the target or his room
    act("There is a brief flash of light, and you sense an unseen presence.", victim, item, NULL, TO_ALL);
    obj_from_char(item);
    if (IS_SET(victim->nact, PLR_NOACCEPT))
    {
        act("$p appears in mid-air, falling to the ground at your feet.", victim, item, NULL, TO_CHAR);
        act("$p appears in mid-air, falling to the ground at $n's feet.", victim, item, NULL, TO_ROOM);
        obj_to_room(item, victim->in_room);
    }
    else
    {
        act("$p appears in mid-air, and is handed to you.", victim, item, NULL, TO_CHAR);
        act("$p appears in mid-air, and is handed to $n.", victim, item, NULL, TO_ROOM);
        obj_to_char(item, victim);
    }

    return TRUE;
}

void wrathkyana_combat_effect(CHAR_DATA *ch, CHAR_DATA *victim)
{
	if (IS_GOOD(victim) && is_affected(ch, gsn_wrathkyana))
	{
		send_to_char("You are {rstricken{x by the holy wrath of Kyana!\n\r", ch);
		act("$n is {rstricken{x by the holy wrath of Kyana!", ch, NULL, NULL, TO_ROOM);
		damage_old(ch, ch, number_range(5, 15), gsn_wrathkyana, DAM_HOLY, FALSE);
	}
}

bool spell_wrathkyana( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;

    if (is_affected(victim,gsn_wrathkyana)) 
    {
        act("$N is already affected by the wrath of Kyana.",ch,NULL,victim,TO_CHAR);
        return FALSE;
    }   

    AFFECT_DATA af = {0};
    af.where     = TO_AFFECTS;
    af.type      = gsn_wrathkyana;
    af.level     = level;
    af.duration  = level/10;
    affect_to_char( victim, &af );


    act("You raise your hand, calling on the holy name of Kyana!",ch,NULL,victim,TO_CHAR);
    act("Motes of light swirl from your hand in a stream, circling around $N!",ch,NULL,victim,TO_CHAR);
    act("The swarm of tiny stars flies into $N's flesh, searing a fine silver mesh as they land!", ch, NULL,victim,TO_CHAR);

    act("$n raises $s hand, pointing at $N as $e utters the words of power!",ch,NULL,victim,TO_NOTVICT);
    act("Motes of light swirl from $n's hand in a stream, circling around $N.",ch,NULL,victim,TO_NOTVICT);
    act("The swarm of tiny stars flies into $N's flesh, searing a fine silver mesh as they land!",ch,NULL,victim,TO_NOTVICT);

    act("$n raises $s hand, pointing at you as $e utters the words of power!",ch,NULL,victim,TO_VICT);
    act("Motes of light swirl from $n's hand in a stream, circling around you.",ch,NULL,victim,TO_VICT);
    act("The swarm of tiny stars sear into you, searing a fine silver mesh into you!",ch,NULL,victim,TO_VICT);

    return TRUE;
}


bool spell_improveddetectevil( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if (IS_AFFECTED(ch, AFF_DETECT_EVIL))
    {
        send_to_char("You can already sense evil.\n\r",ch);
        return FALSE;
    }
    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = level;
    af.modifier  = 0;
    af.location  = APPLY_NONE;
    af.bitvector = AFF_DETECT_EVIL;
    affect_to_char( ch, &af );
    send_to_char( "Your eyes tingle.\n\r", ch );
    return TRUE;
}

bool spell_know_alignment(int sn,int level,CHAR_DATA *ch,void *vo,int target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;

    if (IS_NPC(victim))
    {
	send_to_char("Their intentions are plain, you simply need to 'consider' them.\n\r", ch);
        return FALSE;
    }
    
    if (!IS_PK(ch,victim))
    {
	send_to_char("You can't tell their intentions yet.\n\r",ch);
	return FALSE;
    }

    if (saves_spell(level,ch, victim,DAM_MENTAL))
    {
	send_to_char("You can't tell their intentions.\n\r",ch);
	return TRUE;
    }

    if (IS_GOOD(victim))
	    act("$N has a pure and good aura.", ch, NULL, victim, TO_CHAR);
    else if (IS_EVIL(victim))
            act("$N is the embodiment of pure evil.", ch, NULL, victim, TO_CHAR);
    else
	    act("You sense no particular moral aura from $N.", ch, NULL, victim, TO_CHAR);

    if (( victim->pcdata->ethos == ETH_LAWFUL ))
      act( "$N is lawful in nature.", ch, NULL, victim, TO_CHAR );
    else if (( victim->pcdata->ethos == ETH_NEUTRAL ))
      act( "$N follows the middle path.", ch, NULL, victim, TO_CHAR );
    else if (( victim->pcdata->ethos == ETH_CHAOTIC ))
      act( "$N is a creature of chaos.", ch, NULL, victim, TO_CHAR );

    return TRUE;
}

bool spell_manabarbs( int sn, int level, CHAR_DATA *ch, void *vo,
        int target )
{
    CHAR_DATA * victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if (is_affected(victim, gsn_manabarbs))
    {
        send_to_char("Their magic already pains them.\n\r", ch);
        return FALSE;
    }

    if (saves_spell(level,ch, victim,DAM_MENTAL) && number_bits(3) != 0)
    {
        send_to_char("Their mind resists your manabarbs.\n\r",ch);
        return TRUE;
    }

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = level/10;
    af.location  = 0;
    af.modifier  = 0;
    af.bitvector = 0;
    affect_to_char( victim, &af );

    send_to_char("Their magic will now pain them.\n\r", ch);
    send_to_char("You feel your mind twist upon itself.\n\r", victim);

    return TRUE;
}

bool spell_resonance(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    // Sanity-checks
    if (ch->in_room == NULL)
    {
        bug("Resonance called from null room", 0);
        return false;
    }

    if (target_name[0] == '\0')
    {
        send_to_char("For whose resonance did you wish to search?\n", ch);
        return false;
    }

    // Look for a target
    CHAR_DATA * victim(get_char_world(ch, target_name));
    if (victim == NULL)
    {
        send_to_char("You sense no such person in the world.\n", ch);
        return false;
    }

    act("You hold two fingers to your temple, concentrating on the Weave about you.", ch, NULL, NULL, TO_CHAR);
    act("$n holds two fingers to $s temple, clearly lost in concentration.", ch, NULL, NULL, TO_ROOM);

    // Find a path between the two
    if (victim->in_room == NULL)
    {
        act("You can find no trace of $N's resonance in the Weave.", ch, NULL, victim, TO_CHAR);
        return true;
    }

    RoomPath path(*ch->in_room, *victim->in_room, ch);
    if (!path.Exists())
    {
        act("You can find no trace of $N's resonance in the Weave.", ch, NULL, victim, TO_CHAR);
        return true;
    }

    // Handle special cases of same room and adjacent room
    if (path.StepCount() == 0)
    {
        act("Your concentration breaks as you realize $N is right here!", ch, NULL, victim, TO_CHAR);
        return true;
    }

    if (path.StepCount() == 1)
    {
        std::ostringstream mess;
        mess << "$N's resonance is powerful here, leading " << Direction::DirectionalNameFor(static_cast<Direction::Value>(path.StepDirection(0))) << ".";
        act(mess.str().c_str(), ch, NULL, victim, TO_CHAR);
        return true; 
    }

    // Check for bond of souls
    bool hasBond(false);
    for (AFFECT_DATA * paf(get_affect(ch, gsn_bondofsouls)); paf != NULL; paf = get_affect(ch, gsn_bondofsouls, paf))
    {
        if (paf->modifier == victim->id)
        {
            hasBond = true;
            break;
        }
    }

    // Echo according to distance
    if (path.StepCount() <= 5) act("You sense $N's resonance casually, and realize $E is quite close.", ch, NULL, victim, TO_CHAR);
    else if (path.StepCount() <= 10) act("You sense $N's resonance without difficulty, and realize $E is fairly close.", ch, NULL, victim, TO_CHAR);
    else if (path.StepCount() <= 15) act("You sense $N's resonance with little difficulty, and realize $E is somewhat close.", ch, NULL, victim, TO_CHAR);
    else if (path.StepCount() <= 22) act("You sense $N's resonance, and realize $E is not far off.", ch, NULL, victim, TO_CHAR);
    else if (path.StepCount() <= 29) act("You sense $N's resonance with a little effort, and realize $E is not terribly close.", ch, NULL, victim, TO_CHAR);
    else if (path.StepCount() <= 36) act("You sense $N's resonance with some effort, and realize $E is moderately far off.", ch, NULL, victim, TO_CHAR);
    else if (path.StepCount() <= 43) act("You sense $N's resonance with effort, and realize $E is a bit distant.", ch, NULL, victim, TO_CHAR);
    else if (path.StepCount() <= 50) act("You sense $N's resonance with substantial effort, and realize $E is nowhere nearby.", ch, NULL, victim, TO_CHAR);
    else if (path.StepCount() <= 60) act("You struggle to sense $N's resonance, and realize $E is probably somewhat distant.", ch, NULL, victim, TO_CHAR);
    else if (path.StepCount() <= 75) act("You struggle greatly to sense $N's resonance, and realize $E is fairly far away.", ch, NULL, victim, TO_CHAR);
    else if (path.StepCount() <= 90) act("You can barely make out a tricle of $N's resonance, and realize $E is far away.", ch, NULL, victim, TO_CHAR);
    else act("You cannot trace $N's resonance, and realize $E must be leagues away.", ch, NULL, victim, TO_CHAR);

    if (hasBond)
    {
        std::ostringstream mess;
        mess << "Your bond with $N guides your steps regardless of distance, leading " << Direction::DirectionalNameFor(static_cast<Direction::Value>(path.StepDirection(0))) << ".";
        act(mess.str().c_str(), ch, NULL, victim, TO_CHAR);
        return true;
    }

    // Perform a ley check for direction
    if (Weave::HasWeave(*ch->in_room) && Weave::HasWeave(*victim->in_room))
    {
        std::ostringstream mess;
        mess << "You sense the impact of $N's resonance primarily on the energy flows " << Direction::DirectionalNameFor(static_cast<Direction::Value>(path.StepDirection(0))) << ".";
        act(mess.str().c_str(), ch, NULL, victim, TO_CHAR);
    }

    return true;
}


bool spell_masspeace( int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *vch;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    for (vch = char_list;vch != NULL;vch = vch->next)
    {
        if (!vch || !vch->in_room)
            continue;
        if (vch->in_room->area == ch->in_room->area)
        {
            send_to_char("A gentle wave of calming energy drifts through the area.\n\r", vch);

            if (vch->fighting != NULL)
                stop_fighting_all(vch);

            if (!is_affected(vch, gsn_masspeace)
             && !saves_spell(level, ch, vch, DAM_OTHER)
             && vch->in_room
             && ch->in_room)
            {
                af.where     = TO_AFFECTS;
                af.type      = sn;
                af.level     = level;
                af.duration  = number_range(0,1);
                af.location  = 0;
                af.modifier  = 0;
                af.bitvector = 0;
                affect_to_char(vch, &af);
                send_to_char("A strange feeling of peace overwhelms you.\n\r", vch);
            }
        }
    }
    return TRUE;
}

bool spell_poschan ( int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
    AFFECT_DATA *paf, *paf_next;
    int dur = 0;

    if (target_name[0] == '\0')
    {
        send_to_char("You need to specify whether you want to open or close the channel.\n\r",ch);
        return FALSE;
    }

    if (!str_cmp(target_name, "open")) 
    {
        if (IS_OAFFECTED(ch,AFF_POSCHAN))
        {
            send_to_char("You already have a channel open.\n\r",ch);
            return FALSE;
        }
        else
        {     
            if (is_affected(ch,gsn_poschan))
            {    
                send_to_char("You cannot open a channel again so soon.\n\r",ch);
                return FALSE;
            }
            send_to_char("You open a channel to the positive plane of energy.\n\r",ch);
            send_to_char("The powerful energy of life itself floods your being!\n\r",ch);
            send_to_char("Every nerve tingling, you gasp at the power your body now holds.\n\r",ch);
            act("A soft blue glow luminesces inside $n's body.",ch,NULL,NULL,TO_ROOM); 
            act("$n gasps, basking in rapture as $e draws on the energy of life itself.",ch,NULL,NULL,TO_ROOM);

            af.where     = TO_OAFFECTS;
            af.type      = gsn_poschan;
    	    af.level     = level;
    	    af.modifier  = 0;
    	    af.location  = 0;
    	    af.duration  = -1;
    	    af.bitvector = AFF_POSCHAN;
    	    affect_to_char(ch,&af);

            af.modifier = level/2;
            af.location = APPLY_HITROLL;
            af.bitvector = 0;
            affect_to_char(ch,&af);

            af.location = APPLY_DAMROLL;
            affect_to_char(ch,&af);

            af.modifier = 50 + level;
            af.location = APPLY_HIT;
            affect_to_char(ch,&af);

	    ch->hit += (50 + level);

            return TRUE;    
        }
    }
       
    if (!str_cmp(target_name, "close")) 
    {
        if (!IS_OAFFECTED(ch,AFF_POSCHAN))
        { 
            send_to_char("You have no connection to the power of the spirit realm.\n\r",ch);
            return FALSE;
        }
        else 
        {
            for ( paf = ch->affected; paf != NULL; paf = paf_next )
            {
                paf_next    = paf->next;
                if (paf->bitvector == AFF_POSCHAN) 
                    dur = paf->modifier;
            }

            affect_strip (ch,gsn_poschan);

            af.where     = TO_AFFECTS;
            af.type      = gsn_poschan;
            af.level     = level;
            af.modifier  = -50*dur;
            af.location  = APPLY_HIT;
            af.duration  = dur;
            af.bitvector = 0;
            affect_to_char(ch,&af);

            af.modifier = -50*dur;
            af.location = APPLY_MANA;
            affect_to_char(ch,&af);

            if (saves_spell(level, NULL, ch, DAM_HOLY))
            {
                send_to_char("You seal off the channel to the positive plane of energy.\n\r",ch);
                send_to_char("The power lent to you rushes from your body, dissipating rapidly.\n\r",ch);
                send_to_char("Your body feels weak and tired, stripped of the power of the channel.\n\r",ch);
	        act("A soft blue glow pulses in $n, then fades away.",ch,NULL,NULL,TO_ROOM);
  	        act("$n stumbles in exhaustion, but catches $mself.",ch,NULL,NULL,TO_ROOM);
             }
             else
             {
                send_to_char("You seal off the channel to the positive plane of energy.\n\r",ch);
                send_to_char("The power lent to you rushes from your body, dissipating rapidly.\n\r",ch);
                send_to_char("You collapse from the prolonged strain of holding the channel open!\n\r",ch);
                act("A soft blue glow pulses in $n, then fades away.",ch,NULL,NULL,TO_ROOM);
  	        act("$n stumbles in exhaustion, and collapses.",ch,NULL,NULL,TO_ROOM);

		switch_position(ch, POS_SLEEPING);                
	        af.type      = gsn_lucid;	
                af.where     = TO_AFFECTS; 
                af.location  = APPLY_HIDE;
                af.duration  = dur/2;
                af.bitvector = AFF_SLEEP;
                affect_to_char(ch,&af);
              }   
          }   
         return TRUE;
    }
    send_to_char("You need to specify whether you want to open or close the channel.\n\r",ch);
    return TRUE;
}   

bool spell_radiance(int sn,int level,CHAR_DATA *ch,void *vo,int target)
{
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if (IS_OAFFECTED(ch,AFF_RADIANCE))
    {
        send_to_char("You are already surrounded by a brilliant radiance.\n\r",ch);
        return FALSE;
    }

    send_to_char("You are surrounded by a brilliant radiance.\n\r",ch);
    act("As $n completes $s spell, a brilliant radiance surrounds $m.",ch,NULL,NULL,TO_ROOM);

    af.type      = gsn_radiance;
    af.where     = TO_OAFFECTS;
    af.location  = 0;
    af.modifier  = 0;
    af.duration  = level * 2;
    af.bitvector = AFF_RADIANCE;
    affect_to_char(ch,&af);
    return TRUE;
}

bool spell_raylight( int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim;
    char arg[MAX_INPUT_LENGTH];
    char *direction;
    ROOM_INDEX_DATA *pRoom = NULL;
    int dir = -1, dam, i;

    if (!ch->in_room)
        return FALSE;

    direction = one_argument(target_name, arg);

    if ((arg[0] == '\0') && !ch->fighting)
    {
        send_to_char("Who are you trying to hit with your ray of light?\n\r", ch);
        return FALSE;
    }

    if (direction[0] == '\0')
        pRoom = ch->in_room;
    else
    {
        if (ch->fighting)
        {
            send_to_char("You can't shoot a ray of light into another room while fighting!\n\r", ch);
            return FALSE;
        }

        for (i = 0; i < 6; i++)
            if (!str_prefix(direction, dir_name[i]))
                dir = i;

        if (dir == -1)
        {
            send_to_char("Invalid direction.\n\r",ch);
            return FALSE;
        }

        if (!ch->in_room->exit[dir]
         || IS_SET(ch->in_room->exit[dir]->exit_info, EX_CLOSED)
         || IS_SET(ch->in_room->exit[dir]->exit_info, EX_WALLED)
         || IS_SET(ch->in_room->exit[dir]->exit_info, EX_WALLOFFIRE)
         || IS_SET(ch->in_room->exit[dir]->exit_info, EX_WALLOFVINES)
         || IS_SET(ch->in_room->exit[dir]->exit_info, EX_FAKE))
        {
            send_to_char("You can't shoot a ray of light in that direction!\n\r",ch)
;
            return FALSE;
        }

        pRoom = ch->in_room->exit[dir]->u1.to_room;
    }

    if (pRoom == NULL)
    {
        send_to_char("You cannot shoot your ray of light in that direction!\n\r",ch);
        return FALSE;
    }

    if (room_is_affected(ch->in_room, gsn_smoke)
     || room_is_affected(pRoom, gsn_smoke))
    {
        send_to_char("The thick smoke wafting by blocks you from targetting your ray.\n\r",ch);
        return FALSE;
    }

    if (pRoom == ch->in_room)
    {
        if (arg[0] == '\0')
        {
            if (ch->fighting)
                victim = ch->fighting;
            else
            {
                send_to_char("Shoot a ray of light at whom?\n\r", ch);
                return FALSE;
            }
        }
        else if ((victim = get_char_room(ch,arg)) == NULL)
        {
            send_to_char("You don't see them here.\n\r",ch);
            return FALSE;
        }
    }
    else
    {
        victim = get_char_room(ch, pRoom, arg);
        if (!victim)
        {
            send_to_char("You can't seem to see that person in that direction.\n\r",ch);
            return FALSE;
        }
    }

    if (is_safe_spell(ch, victim, FALSE))
        return FALSE;

    dam = dice(level, 4) + dice((level * 2) / 3, 1);

    if (saves_spell(level, ch, victim, DAM_LIGHT))
        dam /= 2;

    if (pRoom != ch->in_room)
        dam /= 2;

    if (room_is_affected(pRoom, gsn_globedarkness))
        dam /= 2;

    /* 1/2 damage if they save, 1/2 damage again if it's one room away. */

    act("You throw your hands wide and conjure a powerful ray of light!",ch, NULL,NULL,TO_CHAR); 

    if (pRoom == ch->in_room)
    {
        act("$n throws $s hands wide, conjuring a powerful ray of light, which streaks towards $N.", ch, NULL, victim,TO_NOTVICT);
        act("$n throws $s hands wide, and a ray of light streaks through the air towards you!", ch, NULL, victim, TO_VICT);
    }
    else
    {
        char buf[MAX_STRING_LENGTH];

        sprintf(buf, "$n throws $s hands wide, conjuring a powerful ray of light, which streaks %s!", (dir == 0 ? "northwards" 
                      : dir == 1 ? "eastwards"
                      : dir == 2 ? "southwards"
                      : dir == 3 ? "westwards"
                      : dir == 4 ? "upwards"
                      : dir == 5 ? "down below you" : "away"));
        act(buf, ch, NULL, NULL, TO_ROOM);
        act("A ray of light appears and streaks towards $n suddenly!", victim, NULL, NULL, TO_ROOM);
        act("A ray of light appears and streaks towards you!", victim, NULL, NULL, TO_CHAR);
    }

    if (is_affected(victim,gsn_reflectiveaura))
        if (number_percent() < get_skill(victim,gsn_reflectiveaura))
	{
	    act("The ray of light refracts away from $N, and a smaller ray returns to strike you!",ch,NULL,victim,TO_CHAR);
	    act("The ray of light refracts away from you, and a smaller ray returns to strike $n!",ch,NULL,victim,TO_VICT);
	    act("The ray of light refracts away from $N, and a smaller ray returns to strike $n!",ch,NULL,victim,TO_VICTROOM);
	    damage_old(ch,ch,dam/2,sn,DAM_LIGHT,TRUE);
	}
	else
	{
	    act("The ray of light partially splinters before it reaches $N.",ch,NULL,victim,TO_CHAR);
	    act("The ray of light partially splinters before it reaches you.",ch,NULL,victim,TO_VICT);
	    act("The ray of light partially splinters before it reaches $N.",ch,NULL,victim,TO_ROOM);
	}

    if (saves_spell(level, ch, victim, DAM_LIGHT))
        dam /= 2;

    if ((pRoom != ch->in_room) && IS_NPC(victim) 
    && ((IS_SET(victim->act, ACT_SENTINEL)
    || IS_SET(victim->act, ACT_NOTRACK))) 
    && (victim->hit < victim->max_hit))
        dam = 0;
    
    if (pRoom != ch->in_room
      && check_defensiveroll(victim))
    {
	send_to_char("You roll as the ray of light strikes you, lessening its affect.\n\r",victim);
	dam /= 2;
    }
    damage_old( ch, victim, dam, sn,DAM_LIGHT,TRUE);

    return TRUE;
}

bool spell_ritenathli( int sn, int level, CHAR_DATA *ch, void *vo,int target)
{
    CHAR_DATA *vch, *vch_next;
    char buf[MAX_STRING_LENGTH];
    int dam;
    
    send_to_char("You intone the words of the holy rite.\n\r",ch);
    send_to_char("Gesturing carefully, you trace the sacred symbol of Nathli in the air.\n\r",ch);
    send_to_char("You raise your arms, and a shaft of pure white light rises through you, arcing out all around you!\n\r",ch);

    act("$n intones a soft, melodic chant.",ch,NULL,NULL,TO_ROOM);
    act("$n gestures, tracing an intricate figure in the air.",ch,NULL,NULL,TO_ROOM);
    act("$n raises $s arms, and a pure white light wells up in $m, arcing out of $s hands, and striking all around $m!", ch,NULL,NULL,TO_ROOM);


    for (vch = ch->in_room->people; vch != NULL; vch = vch_next)
    {
        // Disqualify certain targets
        vch_next = vch->next_in_room;
        if (ch == vch || is_safe_spell(ch, vch, true) 
        || ((IS_AFFECTED(vch, AFF_WIZI) || IS_IMMORTAL(vch)) && !IS_IMMORTAL(ch)))
            continue;

        // Calculate damage and check for undead/revenant
        dam = dice(level, 6) + level;
        if (IS_NPC(vch))
        {
            if (!IS_SET(vch->act, ACT_UNDEAD))
                continue;
        }
        else if (number_percent() <= get_skill(vch, gsn_revenant)) dam = (dam * 4) / 10;
        else continue;

        // Handle autoyell
        if (!IS_NPC(vch) && (!ch->fighting || !vch->fighting))
        {
            if (can_see(vch, ch)) sprintf(buf, "Help!  %s is attacking me!", PERS(ch, vch));
            else sprintf(buf, "Help!  Someone is attacking me!");
            do_autoyell(vch, buf);
        }

        // Check for save and insta-kill
        check_killer(ch, vch);
        if (saves_spell(level, ch, vch, DAM_HOLY))
           damage_old(ch, vch, dam / 2, sn, DAM_HOLY, TRUE);
        else if (number_percent() < 3 && IS_NPC(vch) && !IS_SET(vch->act, ACT_NOSUBDUE))
        {
            act("Your holy power strikes $N, and they are reduced to dust!",ch,NULL,vch,TO_CHAR);
            act("$N is stricken by the power of the Rite of Nalthi, and is reduced to dust!",NULL,NULL,vch,TO_ROOM);
            kill_char(vch, ch);
        } 
        else
            damage_old(ch, vch, dam, sn, DAM_HOLY, TRUE);
    }
    return TRUE;
}

void apply_runeofspirit_cooldown(CHAR_DATA * ch)
{
    AFFECT_DATA af = {0};
    af.where    = TO_AFFECTS;
    af.type     = gsn_runeofspirit;
    af.level    = ch->level;
    af.duration = 17 - UMAX(0, (get_skill(ch, gsn_runeofspirit) - 70) / 6);
    affect_to_char(ch, &af);
}

bool spell_runeofspirit( int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    ROOM_INDEX_DATA *room;
    AFFECT_DATA af = {0};
    unsigned long num;

    if (ch->in_room == NULL)
         return FALSE;

    if (is_affected(ch, sn))
    {
        send_to_char("You are not ready to scribe another rune of spirit yet.\n\r", ch);
        return FALSE;
    }

    if (IS_SET(ch->in_room->room_flags, ROOM_NOGATE))
    {
	send_to_char("You cannot place a rune of spirit here.\n\r", ch);
	return FALSE;
    }

    if (target_name[0] == '\0')
    {
        room = ch->in_room;

        if (room_is_affected(room, sn))
        {
            send_to_char("Another rune of spirit already dominates the weave here.\n\r", ch);
            return FALSE;
        }

        af.where     = TO_ROOM_AFF;
        af.type      = sn;
        af.level     = level;
        af.location  = 0;
        af.modifier  = num = number_range(1, 10000);
        af.duration  = level*2;
        af.bitvector = 0;
        affect_to_room(room, &af);
        af.where     = TO_AFFECTS;
        af.type      = sn;
        af.level     = level;
        af.duration  = level*2;
        af.location  = APPLY_HIDE;
        af.modifier  = num;
        af.bitvector = 0;
        affect_to_char(ch, &af);

        // Penalize mana while this rune exists
        af.location  = APPLY_MANA;
        af.modifier  = -200;
        affect_to_char(ch, &af);
        ch->mana = UMIN(ch->mana, ch->max_mana);

        act("$n concentrates and marks this place with a rune of spirit.", ch, NULL, NULL, TO_ROOM);
        act("You chant softly, marking this place with a rune of spirit.", ch, NULL, NULL, TO_CHAR);

        return TRUE;
    }

    OBJ_DATA * obj;
    if ((obj = get_obj_list(ch, target_name, ch->in_room->contents)) == NULL)
    {
        if ((obj = get_obj_list(ch, target_name, ch->carrying)) == NULL)
        {
	        send_to_char("You don't see that here.\n\r",ch);
            return FALSE;
        }
    }

    if (obj->worn_on != 0)
    {
	send_to_char("You can't concentrate on that while you're using it.\n\r",ch);
	return FALSE;
    }
    if (obj_is_affected(obj, sn))
    {
        act("Another rune of spirit rests upon $p already.", ch, obj, NULL, TO_CHAR);
        return FALSE;
    }

    af.where     = TO_OBJECT;
    af.type      = sn;
    af.level     = level;
    af.duration  = level*2;
    af.location  = 0;
    af.modifier  = num = number_range(10000, 19999);
    af.bitvector = 0;
    affect_to_obj(obj, &af);

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = level*2;
    af.location  = APPLY_HIDE;
    af.modifier  = num;
    af.bitvector = 0;
    affect_to_char(ch, &af);

    // Penalize mana while this rune exists
    af.location  = APPLY_MANA;
    af.modifier  = -200;
    affect_to_char(ch, &af);
    ch->mana = UMIN(ch->mana, ch->max_mana);

    act("$n chants softly, marking $p with a rune of spirit.", ch, obj, NULL, TO_ROOM);
    act("You chant softly, branding $p with a rune of spirit.", ch, obj, NULL, TO_CHAR);

    return TRUE;
}

bool spell_sanctify( int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if (!ch->in_room)
        return FALSE;

    if (is_affected(ch, gsn_sanctify))
    {
        send_to_char("You aren't ready to sanctify an area yet.\n\r", ch);
        return FALSE;
    }
    if (room_is_affected(ch->in_room, gsn_sanctify))
    {
         send_to_char("This place has already been sanctified.\n\r",ch);
         return FALSE;
    }

    if (IS_SET(ch->in_room->room_flags,ROOM_NO_RECALL))
    {
         send_to_char("This place is cursed by the gods, and cannot be sanctified.\n\r",ch);
         return FALSE;
    }

    if (ch->in_room->sector_type == SECT_AIR
     ||   ch->in_room->sector_type == SECT_UNDERWATER
     ||   ch->in_room->sector_type == SECT_WATER_NOSWIM
     ||   ch->in_room->sector_type == SECT_WATER_SWIM)
    {
        send_to_char("There is no ground here to sanctify.\n\r", ch);
        return FALSE;
    }  
    
    //brazen: Ticket #167: delay changed to match other room-affecting spells
    af.where     = TO_ROOM;
    af.type      = gsn_sanctify;
    af.level     = level;
    af.modifier  = 0;
    af.location  = 0;
    af.duration  = 24; 
    af.bitvector = 0;
    affect_to_room(ch->in_room, &af);

    af.where 	= TO_AFFECTS;
    af.duration = 50;
    affect_to_char(ch,&af);

    send_to_char("You open this place to the Weave, sanctifying the ground.\n\r", ch);
    act("As $n finishes $s casting, a soft glow fills the area!\n\r", ch, NULL, NULL, TO_ROOM);

    if (room_is_affected(ch->in_room,gsn_globedarkness) || room_is_affected(ch->in_room, gsn_desecration))
    {
        room_affect_strip(ch->in_room, gsn_globedarkness);
        room_affect_strip(ch->in_room, gsn_desecration);
        act("The darkness flees from the sanctity of this place!",ch,NULL,NULL,TO_CHAR);
        act("The darkness flees from the sanctity of this place!",ch,NULL,NULL,TO_ROOM); 	
    }
    return TRUE;
}




bool spell_soulburn( int sn, int level,
 CHAR_DATA *ch,void *vo,int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    static const int dam_each[] =
    {
         0,
         3,  3,  6,  6,  7,      8,  8,  8,  8,  8,
         9,  9,  9,  9,  9,      11,  11,  11,  11,  11,
         13, 13, 13, 13, 13,    15, 15, 15, 15, 15,
        17, 17, 17, 17, 17,     19, 19, 19, 19, 19,
        21, 21, 21, 21, 21,     23, 23, 23, 23, 23
    };
    int dam;

    level       = UMIN(level, static_cast<int>(sizeof(dam_each)/sizeof(dam_each[0])) - 1);
    level       = UMAX(0, level);
    dam         = number_range( dam_each[level] / 2, dam_each[level] * 2 );
    if ( saves_spell( level, ch, victim,DAM_MENTAL) )
        dam /= 2;
    act("$n's eyes glow as $e glares at $N.", ch, NULL, victim, TO_ROOM);
    damage_old( ch, victim, dam, sn, DAM_MENTAL ,TRUE);
    return TRUE;
}

bool spell_soulflare( int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    CHAR_DATA *vch, *vch_next;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
    char buf[MAX_STRING_LENGTH];
    int dam;

    if (!ch->in_room)
        return FALSE;
    act("You call forth the energy of your own spirit, summoning a flare of pure white light!",ch,NULL,victim,TO_CHAR);
    act("$n raises $s hand, palm outstretched, and a blinding glow flares around you!",ch,NULL,victim,TO_VICT);
    act("$n raises $s hand, palm outstretched, and a blinding glow surrounds $N!",ch,NULL,victim,TO_NOTVICT);

    dam = dice(level, 6);

    for (vch = ch->in_room->people; vch; vch = vch_next)
    {
	vch_next = vch->next_in_room;
	
	if (!IS_IMMORTAL(vch)
	  && !IS_AFFECTED(vch,AFF_WIZI)
	  && (IS_NPC(vch) || IS_PK(ch,vch))
	  && vch != ch
	  && (is_same_group(vch,victim)
	    || vch->fighting == ch
	    || vch->master == victim)
	  && !is_safe_spell(ch,vch,TRUE))
	  
        {
            if (!IS_NPC(vch) && (vch != victim) && (!ch->fighting || !vch->fighting))
            {
                if (can_see(vch, ch))
                    sprintf(buf, "Help!  %s is attacking me!", PERS(ch, vch));
                else
                    sprintf(buf, "Help!  Someone is attacking me!");
                    do_autoyell(vch, buf);
            }
            check_killer(ch, vch);

            if (vch->race == global_int_race_shuddeni)
            {
                send_to_char("The excruciating light of the soul flare sears your flesh!\n\r",vch);
                act("$n screams as $s flesh is seared by the light of the soul flare!",vch,NULL,NULL,TO_ROOM);
	
                if (saves_spell(level, ch, vch, DAM_LIGHT))
	            if (is_affected(vch,gsn_reflectiveaura))
	            {
	                act("The reflective aura around $n diffuses the light from $N's soul flare.",vch,NULL,ch,TO_NOTVICT);
	                act("The reflective aura around $n diffuses the light from your soul flare.",vch,NULL,ch,TO_VICT);
	                act("Your reflective aura diffuses the light from $N's soul flare.",vch,NULL,ch,TO_CHAR);
	            }
                    else
			damage_old(ch, vch, dam / 2, sn, DAM_LIGHT, TRUE);
                else
		{
	            if (is_affected(vch,gsn_reflectiveaura))
	            {
	                act("The reflective aura around $n partially diffuses the light from $N's soul flare.",vch,NULL,ch,TO_NOTVICT);
	                act("The reflective aura around $n partially diffuses the light from your soul flare.",vch,NULL,ch,TO_VICT);
	                act("Your reflective aura partially diffuses the light from $N's soul flare.",vch,NULL,ch,TO_CHAR);
	            }
		    else
			damage_old(ch, vch, dam , sn, DAM_LIGHT, TRUE);
		}
            }   
            else if (!IS_AFFECTED(vch, AFF_BLIND))
            {
	    if (is_affected(vch,gsn_reflectiveaura))
	    {
	        act("The reflective aura around $n diffuses the light from $N's soul flare.",vch,NULL,ch,TO_NOTVICT);
	        act("The reflective aura around $n diffuses the light from your soul flare.",vch,NULL,ch,TO_VICT);
	        act("Your reflective aura diffuses the light from $N's soul flare.",vch,NULL,ch,TO_CHAR);
	    }
                if (!saves_spell(level, ch, vch, DAM_LIGHT) && !IS_SET(vch->imm_flags, IMM_BLIND))
                {
	    	    if (is_affected(vch,gsn_reflectiveaura))
	    	    {
	        	act("The reflective aura around $n partially diffuses the light from $N's soul flare.",vch,NULL,ch,TO_NOTVICT);
	        	act("The reflective aura around $n partially diffuses the light from your soul flare.",vch,NULL,ch,TO_VICT);
	        	act("Your reflective aura partially diffuses the light from $N's soul flare.",vch,NULL,ch,TO_CHAR);
			af.duration = 1;
	    	    }
		    else
			af.duration = level/6;
                    af.where     = TO_AFFECTS;
                    af.type      = sn;
                    af.level     = level;
                    af.location  = APPLY_HITROLL;
                    af.modifier  = -4;
                    af.bitvector = AFF_BLIND;
                    affect_to_char( vch, &af );
                    send_to_char( "You are blinded by the light of the soul flare!\n\r", vch );
                    act("$n appears to be blinded.",vch,NULL,NULL,TO_ROOM);
                }
                else
                     send_to_char("You are unaffected by the light of the soul flare.\n\r",vch);
            }    
        }   
    }   
    return TRUE;
}   

static bool SpeakWithTheDead_Corpse(int sn, int level, CHAR_DATA * ch, OBJ_DATA * corpse)
{
    char buf[MAX_STRING_LENGTH];
    char dir[MAX_STRING_LENGTH];
    char when[MAX_STRING_LENGTH];

    act("You stretch out a hand over $p, and a soft silver glow illuminates it as you reach into the spirit world with your thoughts.", ch, corpse, NULL, TO_CHAR);
    act("$n stretches out a hand over $p, and a soft silver glow begins to illuminate it.", ch, corpse, NULL, TO_ROOM);

    if (corpse->killed_by == NULL)
    {
        send_to_char("A hint of whispering breathes past your ears, but such words as they are are devoid of meaning.\n", ch);
        return true;
    }

    if (corpse->item_type == ITEM_CORPSE_PC)
	    sprintf(buf, "A hollow whisper in your ear speaks of %s dealing a fatal blow and felling %s.", corpse->killed_by, corpse->owner);
    else
    	sprintf(buf,"A hollow whisper in your ear speaks of %s dealing a fatal blow and felling the person who left the corpse.", corpse->killed_by);
    
    act(buf, ch, NULL, NULL, TO_CHAR);
    CHAR_DATA * killer(get_char_world(ch, corpse->killed_by));
    if (killer == NULL)
        return true;

    for (TRACK_DATA * pTrack = ch->in_room->tracks; pTrack != NULL; pTrack = pTrack->next)
    {
        if (pTrack->ch != killer)
            continue;

        sprintf(dir, Direction::NameFor(static_cast<Direction::Value>(pTrack->direction)));
        switch (pTrack->time)
        {
            case 0:  sprintf(when, "only moments ago"); break;
            case 1:  sprintf(when, "only a half hour hour ago");  break;
            default: sprintf(when, "about %d hours ago", pTrack->time / 2); break;
        }

        sprintf(buf, "With a final, faint breath, the toneless voice murmurs that the killer left %s, %s.", dir, when);        
        act(buf, ch, NULL, NULL, TO_CHAR);
        break;
    }

    return true;
}

static bool SpeakWithTheDead_Guide(int sn, int level, CHAR_DATA * ch, CHAR_DATA * shade, char * argument)
{
    // Look up the target
    CHAR_DATA * target(get_char_world(ch, argument));
    if (target == NULL)
    {
        send_to_char("You sense no such being to describe to the shade.\n", ch);
        return false;
    }

    act("You thread a spindle of silver-white light into the spirit realm, trying to catch $N's attention.", ch, NULL, shade, TO_CHAR);
    act("As $n murmurs, a spindle of silver-white light appears, winding its way from $s larynx before trailing off into the air.", ch, NULL, shade, TO_ROOM);
    
    // Check for shade coercing
    if (saves_spell(level, ch, shade, DAM_CHARM))
    {
        act("Your magic nudges $N, but cannot distract $M from $S memories.", ch, NULL, shade, TO_CHAR);
        return true;
    }

    // Check whether a path is possible
    if (target->in_room == NULL)
    {
        act("$N regards you blankly, and you come to realize $E is unable to guide you to your destination.", ch, NULL, shade, TO_CHAR);
        return true;
    }

    // Check whether the target is right here
    if (target->in_room == shade->in_room)
    {
        act("The shade regards you for a long moment, then stares at $N, then turns back to its memories, ignoring you.", ch, NULL, target, TO_CHAR);
        return true;
    }

    // Build the path relative to ch and check that it exists
    RoomPath path(*shade->in_room, *target->in_room, ch);
    if (!path.Exists())
    {
        act("$N regards you hollowly, and you come to realize $E is unable to guide you to your destination.", ch, NULL, shade, TO_CHAR);
        return true;
    }

    // Path is good, set the shade on it
    act("$N regards you solemnly, then slowly begins to drift, a faint silver light trailing behind as $E leads you.", ch, NULL, shade, TO_CHAR);
    AssignNewPath(shade, path);
    StepAlongPath(shade);
    return true;
}

static bool SpeakWithTheDead_Haunt(int sn, int level, CHAR_DATA * ch, CHAR_DATA * shade, char * argument)
{
    char arg[MAX_STRING_LENGTH];
    argument = one_argument(argument, arg);

    // Look up the target
    CHAR_DATA * target(get_char_world(ch, arg));
    if (target == NULL)
    {
        send_to_char("You sense no such being to describe to the shade.\n", ch);
        return false;
    }

    // Sanity-checks
    if (is_safe_spell(ch, target, false))
    {
        act("You cannot harm $N.", ch, NULL, target, TO_CHAR);
        return false;
    }

    if (is_affected(target, gsn_shadeswarm))
    {
        act("$N is already haunted.", ch, NULL, target, TO_CHAR);
        return false;
    }

    // Check for a circle of protection
    if (target->in_room != NULL && is_symbol_present(*target->in_room, OBJ_VNUM_SYMBOL_PROTECT))
    {
        act("$N seems to be somehow protected.", ch, NULL, target, TO_CHAR);
        return false;
    }

    // Check for an object held by the target
    OBJ_DATA * obj(NULL);
    if (argument[0] == '\0')
    {
        act("You must specify an object once held by $N, or else the shade will have no power over $M.", ch, NULL, target, TO_CHAR);
        return false;
    }

    // Argument was supplied; look for the object
    obj = get_obj_carry(ch, argument, ch);
    if (obj == NULL)
    {
        send_to_char("You are not carrying that.\n", ch);
        return false;
    }

    if (IS_SET(obj->extra_flags[0], ITEM_NODESTROY))
    {
        act("$p cannot be dissolved into spirit essence.", ch, obj, NULL, TO_CHAR);
        return false;
    }

    // Make sure the object was held by the target
    bool found(false);
    for (unsigned int i(0); i < MAX_LASTOWNER && !found; ++i)
    {
        if (!str_cmp(obj->lastowner[i], IS_NPC(target) ? target->short_descr : target->name))
            found = true;
    }

    if (!found)
    {
        act("$p was not recently held by $N.", ch, obj, target, TO_CHAR);
        return false;
    }

    act("You thread a spindle of sickly pale light into the spirit realm, trying to catch $N's attention.", ch, NULL, shade, TO_CHAR);
    act("$n murmurs as $e holds $p aloft, and a spindle of sickly pale light appears to stretch from $s larynx, trailing off into the air.", ch, obj, shade, TO_ROOM);

    // Check for shade coercing
    if (saves_spell(level, ch, shade, DAM_CHARM))
    {
        act("Your magic nudges $N, but cannot distract $M from $S memories.", ch, NULL, shade, TO_CHAR);
        return true;
    }

    // Echoes are different if in the same room
    act("$p suddenly dissolves in $n's hand!", ch, obj, NULL, TO_ROOM);
    if (shade->in_room == target->in_room)
    {
        act("The shade regards you hollowly as $p dissolves into spirit essence, then suddenly dives towards $N, wailing as it vanishes within $M!", ch, obj, target, TO_CHAR);

        for (CHAR_DATA * echoChar(ch->in_room->people); echoChar != NULL; echoChar = echoChar->next_in_room)
        {
            if (!can_see(echoChar, shade) || echoChar == ch)
                continue;
            
            if (echoChar == target)
                act("$N suddenly wails and flies at you, weaving through you before settling inside!", target, NULL, shade, TO_CHAR);
            else
                act("A shade suddenly wails, diving towards $N and vanishing within!", echoChar, NULL, target, TO_CHAR);
        }
    }
    else
    {
        act("The shade regards you hollowly as $p dissolves into spirit essence, then suddenly vanishes, seeking out $N!", ch, obj, target, TO_CHAR);
        for (CHAR_DATA * echoChar(ch->in_room->people); echoChar != NULL; echoChar = echoChar->next_in_room)
        {
            if (!can_see(echoChar, shade) || echoChar == ch)
                continue;

            act("$N suddenly winks out of existence!", echoChar, NULL, shade, TO_CHAR);
        }
    }

    send_to_char("A shiver runs down your spine as a cold presence enters your very being, and it slowly dawns on you that you are haunted!\n", target);

    // Now apply the effect
    AFFECT_DATA af = {0};
    af.where    = TO_AFFECTS;
    af.type     = gsn_shadeswarm;
    af.modifier = UMIN(20, 1 + shade->level / 3);
    af.duration = shade->timer / 15;
    af.level    = level;
    affect_to_char(target, &af);

    // Clean up the object and shade
    extract_obj(obj);
    extract_char(shade, true);
    return true;
}

bool spell_speakwiththedead( int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    // Check for no argument
    if (target_name[0] == '\0')
    {
        send_to_char("You must be more specific when speaking with the dead.\n", ch);
        return false;
    }

    // Check for a corpse
    OBJ_DATA * corpse(get_obj_here(ch, target_name));
    if (corpse != NULL && (corpse->item_type == ITEM_CORPSE_PC || corpse->item_type == ITEM_CORPSE_NPC))
        return SpeakWithTheDead_Corpse(sn, level, ch, corpse);

    // Check for a shade
    char arg[MAX_STRING_LENGTH];
    char * argument(one_argument(target_name, arg));
    CHAR_DATA * shade(get_char_room(ch, arg));
    if (shade == NULL || !IS_NPC(shade) || !IS_SET(shade->nact, ACT_SHADE))
    {
        send_to_char("You see neither corpse nor shade by that name here.\n", ch);
        return false;
    }

    // Check for type of speaking
    argument = one_argument(argument, arg);
    if (!str_prefix(arg, "guide")) return SpeakWithTheDead_Guide(sn, level, ch, shade, argument);
    if (!str_prefix(arg, "haunt")) return SpeakWithTheDead_Haunt(sn, level, ch, shade, argument);

    // Unknown argument
    act("You doubt $N would understand that.", ch, NULL, shade, TO_CHAR);
    return false;
}

bool spell_spiritblock( int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    act("You spindle out wisps of energy to bind $N's spirit!",ch, NULL, victim, TO_CHAR);
    act("$n spindles out wisps of energy to bind $N's spirit!",ch, NULL, victim, TO_NOTVICT);
    act("$n spindles out wisps of energy to bind your spirit!",ch, NULL, victim, TO_VICT);

    if (saves_spell(level, ch, victim, DAM_HOLY) && number_bits(3) != 0)
    {
        send_to_char("They resist your spell.\n\r",ch);
        return TRUE;
    }

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = level / 4;
    af.location  = number_range(1, 5);
    af.modifier  = -1 * (level/11);
    af.bitvector = 0;
    affect_to_char( victim, &af );
    send_to_char( "You feel your breath quicken and your muscles tighten, and you can't think straight.\n\r", victim );
    act("$n looks trapped and confused.",victim, NULL, NULL, TO_ROOM);

    return TRUE;
}

bool spell_spiritbond( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
        CHAR_DATA *victim = (CHAR_DATA *)vo;
        AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

        if (!is_same_group(ch, victim) && victim->leader != ch)
        {
          send_to_char("They are not following you or grouped with you.\n\r", ch);
          return FALSE;
        }

        if (ch == victim)
        {
          send_to_char("You cannot spiritbond yourself.\n\r", ch);
          return FALSE;
        }

        if (is_affected(victim, gsn_spiritbond))
        {
          send_to_char("They already have a spiritbond.\n\r", ch);
          return FALSE;
        }

        if (is_affected(ch, gsn_spiritbond))
        {
          send_to_char("You are already locked in a spiritbond.\n\r", ch);
          return FALSE;
        }

        act("You feel a strange link with $N form.", ch, NULL, victim, TO_CHAR);        act("You feel a strange link with $n form.", ch, NULL, victim, TO_VICT);        act("$n and $N seem surprised for a moment.", ch, NULL, victim, TO_NOTVICT);


    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = level/2;
    af.location  = APPLY_HIDE;
    af.modifier  = ch->id;
    af.bitvector = 0;
    affect_to_char( victim, &af );

    af.modifier	 = victim->id;
    affect_to_char( ch, &af );

    af.location  = APPLY_HIT;
    af.modifier  = level*2;
    affect_to_char( victim, &af );
    victim->hit += level*2;
    affect_to_char( ch, &af );
    ch->hit += level*2;

    af.location  = APPLY_MANA;
    af.modifier  = level*2;
    affect_to_char( victim, &af );
    victim->mana += level*2;
    affect_to_char( ch, &af );
    ch->mana += level*2;

    return TRUE;
}

bool spell_spirit_sanctuary(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
    CHAR_DATA *vch, *vch_next;
    ROOM_INDEX_DATA *sanc;

    if (!ch->in_room)
	return FALSE;

    if (is_affected(ch, sn))
    {
	send_to_char("You cannot yet form another spirit sanctuary.\n\r", ch);
	return FALSE;
    }

    sanc = new_room_area(ch->in_room->area);

    sanc->name		= str_dup("A Spirit Sanctuary");
    sanc->description   = str_dup("You are inside of a small, hemispherical area.  Soft, warm light surrounds\n\rthe sanctuary, and illuminates its comfortable interior.  A feeling of\n\rtranquility permeates this place, peaceful in its silence.\n\r");
    sanc->room_flags	= ROOM_NOGATE|ROOM_INDOORS|ROOM_NOSUM_TO|ROOM_NOSUM_FROM|ROOM_NOWHERE|ROOM_NOWEATHER|ROOM_SAFE|ROOM_NOYELL;
    sanc->sector_type	= SECT_INSIDE;
    sanc->clan		= ch->in_room->clan;

    if (IS_SET(ch->in_room->room_flags, ROOM_NO_RECALL))
	SET_BIT(sanc->room_flags, ROOM_NO_RECALL);

    af.where     = TO_ROOM;
    af.type      = sn;
    af.level     = level;
    af.modifier  = 0;
    af.location  = APPLY_NONE;
    af.duration  = 5 + (ch->level / 25);
    af.bitvector = 0;
    affect_to_room(sanc, &af);

    af.where     = TO_AFFECTS;
    af.duration  = 100;
    affect_to_char(ch, &af);

    for (vch = ch->in_room->people; vch; vch = vch_next)
    {
	vch_next = vch->next_in_room;

	if (is_same_group(ch, vch))
	{
	    act("A soft glow surrounds $n, and $e fades from view.", vch, NULL, NULL, TO_ROOM);
	    send_to_char("A soft glow encompasses you, and you feel your surroundings change.\n\r", vch);

	    if (vch->in_room->vnum != 0)
		vch->was_in_room = vch->in_room;

	    global_linked_move = TRUE;
	    char_from_room(vch);
	    char_to_room(vch, sanc);
	    act("$n shimmers into existence.", vch, NULL, NULL, TO_ROOM);
	    do_look(vch, "auto");
	}
    }

    return TRUE;
}


bool spell_spiritshield(int sn,int level,CHAR_DATA *ch,void *vo, int target)
{
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
    CHAR_DATA *vch;

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = 24;
    af.location  = 0;
    af.modifier  = 0;
    af.bitvector = 0;

    if (IS_TRUE_EVIL(ch))
	send_to_char("The spirit shield has no effect on you.\n\r",ch);
    
    for (vch = ch->in_room->people; vch != NULL; vch = vch->next_in_room)
    {
	if (is_same_group(ch, vch))
	{
	    if (!IS_TRUE_EVIL(vch))
	    {
	    	act("A warm glow surrounds you.", vch, NULL, NULL, TO_CHAR);
	        if (!is_affected(vch, gsn_spiritshield))
   	        {
          	    affect_to_char(vch, &af);
	        }
	    }
	}
    }
			
    return TRUE;
}


bool spell_spiritstone( int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
OBJ_DATA *stone;
AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

        if (is_affected(ch, gsn_spiritstone))
        {
          send_to_char("You are not ready to create another stone yet.\n\r", ch);
          return FALSE;
        }

        stone = create_object(get_obj_index(OBJ_VNUM_SPIRIT_STONE), level);
        act("$n focuses, and a small glowing stone forms in $s hand.", ch, NULL, NULL, TO_ROOM);
        act("You focus, forming a stone out of pure energy from within.", ch, NULL, NULL, TO_CHAR);

        stone->timer = (level+10)/2;
	stone->value[0] = level * 7;

        af.where     = TO_AFFECTS;
        af.type      = sn;
        af.level     = level;
        af.duration  = level/2;
        af.location  = 0;
        af.modifier  = 0;
        af.bitvector = 0;
        affect_to_char(ch, &af);

        obj_to_char(stone, ch);

    return TRUE;
}

bool spell_spiritwrack( int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    int dam;
    int mod = aura_grade(victim);
    if (mod <= 0)
    {
        act("$N is unaffected by your spiritwrack!", ch, NULL, victim, TO_CHAR);
        act("You are unaffected by $n's spiritwrack!", ch, NULL, victim, TO_VICT);
        act("$N is unaffected by $n's spiritwrack!", ch, NULL, victim, TO_NOTVICT);
        return TRUE;
    }
    
    dam = dice( level, 7 );
    if (mod == 1) dam -= (REDAURA - effective_karma(*victim)) / 20;
    else dam += effective_karma(*victim) / 200;
       
    // Check for litany of mortification; if not present, check for saves
    bool saved(false);
    if (perform_litany_skill_check(ch, victim, Litany_Mortification, 5))
        act("You mortify $N with your magic, bypassing $S defenses!", ch, NULL, victim, TO_CHAR);
    else if (saves_spell(level, ch, victim, DAM_HOLY))
    {
        dam = dam / 2;
        saved = true;
    }

    // Handle quintessence rush boost
    if (is_quintessence_rushing(ch))
    {
        dam = (dam * 5) / 4;
        if (!saved && number_bits(1) == 0)
        {
            act("You stagger, dazed by the raw intensity of the holy magics!", victim, NULL, NULL, TO_CHAR);
            act("$n staggers, dazed by the raw intensity of the holy magics!", victim, NULL, NULL, TO_ROOM);
            WAIT_STATE(victim, UMAX(victim->wait, PULSE_VIOLENCE));
        }
    }

    damage_old( ch, victim, UMAX(1, dam), sn, DAM_HOLY,TRUE);
    return TRUE;
}

bool spell_subdue( int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if ( is_affected( victim, sn ) )
    {
        if (victim == ch)
          send_to_char("You already feel mellow.\n\r",ch);
        else
          act("$N is already mellowed.",ch,NULL,victim,TO_CHAR);
        return FALSE;
    }

    if (IS_NPC(victim) && IS_SET(victim->act, ACT_NOSUBDUE))
        {
        send_to_char("They are too powerful to subdue.\n\r", ch);
        return FALSE;
        }

    if ( saves_spell( level, ch, victim, DAM_MENTAL) )
        {
        send_to_char("Their mind resists your spell.\n\r", ch);
        return TRUE;
        }

    af.where     = TO_NAFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = level/8;
    af.modifier  = 0;
    af.location  = 0;
    af.bitvector = AFF_SUBDUE;
    affect_to_char( victim, &af );
    send_to_char( "You feel mellow.\n\r", victim );
    if ( ch != victim )
        act("$N is mellowed by your magic.",ch,NULL,victim,TO_CHAR);


 stop_fighting_all(victim);
 if (IS_NPC(victim) && victim->tracking)
        victim->tracking = NULL;

    return TRUE;
}

bool spell_truesight( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if (is_affected(ch, sn))
    {
        send_to_char("You are already affected by truesight.\n\r",ch);
        return FALSE;
    }

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = level/7;
    af.modifier  = 0;
    af.location  = APPLY_HIDE;
    af.bitvector = AFF_DETECT_EVIL;
    affect_to_char( ch, &af);
// brazen: ticket #362: truesight interferes with detect invis
// Removed detect invis from truesight
    af.location  = APPLY_HIDE;
    af.bitvector = AFF_DETECT_GOOD;
    affect_to_char( ch, &af);
 
    af.modifier  = 7;
    af.location  = APPLY_HITROLL;
    af.bitvector = 0;
    affect_to_char (ch, &af);

// brazen: The way IMM_BLIND was added before did not affect characters
// properly, because it was applied TO_AFFECTS along with the hitroll bonus
// instead of TO_WHERE. Separated this and added an additional hidden affect 
// to work as intended.
    af.where     = TO_IMMUNE;
    af.location  = APPLY_HIDE;
    af.bitvector = IMM_BLIND;
    affect_to_char (ch, &af);
    
    send_to_char( "The veils of deception fall away from your eyes.\n\r",ch);
    affect_strip(ch, gsn_blindness);

    return TRUE;
}

bool spell_unshackle( int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *vch = (CHAR_DATA *) vo;
    CHAR_DATA *mch;

    if (!vch || vch == ch)
    {
	send_to_char("What do you wish to unshackle?\n\r", ch);
	return FALSE;
    }

    act("$n calls upon the forces of spirit to free the enslaved.", ch, NULL, NULL, TO_ROOM);
    act("You call upon the forces of spirit to free the enslaved.", ch, NULL, NULL, TO_CHAR);

    if (!IS_AFFECTED(vch, AFF_CHARM) || !vch->master)
    {
	act("$N is not being controlled by another.", ch, NULL, vch, TO_CHAR);
	return TRUE;
    }

    mch = vch->master;

    if (is_safe_spell(ch, mch, FALSE))
    {
	act("$N is unaffected by your magics!", ch, NULL, vch, TO_CHAR);
	return TRUE;
    }

    if (saves_spell(level, ch, vch, DAM_HOLY) || number_percent() <= Drakes::SpecialCount(*vch, Drakes::Loyal))
    {
        act("$N resists your spell.", ch, NULL, vch, TO_CHAR);
        do_emote(vch, "screams and attacks!");
        check_killer(ch, vch);
        multi_hit(vch, ch, TYPE_UNDEFINED);
        return TRUE;
    }

    act("$n is freed from bondage!", vch, NULL, NULL, TO_ROOM);
    act("You are freed from bondage!", vch, NULL, NULL, TO_CHAR);
    if (mch->pet == vch)
        mch->pet = NULL;

    if (IS_NPC(vch) && IS_SET(vch->act, ACT_ILLUSION))
    {
        act("$N vanishes as $n's hold on it slips away.", ch, NULL, vch, TO_ROOM);
        act("Your control of $N slips away, and it slowly fades away.", mch, NULL, vch, TO_CHAR);
        extract_char(vch, TRUE);
        return TRUE;
    }

    stop_fighting_all(vch);
    if (IS_NPC(vch) && is_affected(vch, gsn_demoniccontrol))
    {
        act("You feel your control over $N fade.", mch, NULL, vch, TO_CHAR);

        vch->tracking = mch;
        vch->demontrack = mch;

        if (vch->in_room == mch->in_room)
        {
            set_fighting(vch, mch);
            act("$n screams in fury and attacks!", vch, NULL, NULL, TO_ROOM);
            multi_hit(vch, mch, TYPE_UNDEFINED);
        }
    }
    
    // Check for puppetmaster (aka reshackle)
    if (IS_NPC(vch) && !vch->demontrack && ch->pet == NULL && !saves_spell(ch->level, ch, vch, DAM_NEGATIVE))
    {
        if (number_percent() <= get_skill(ch, gsn_reshackle))
        {
            // Successful reshackle
            check_improve(ch, vch, gsn_reshackle, true, 2);

            act("Bands of dark power loop around $N, binding $M to you!", ch, NULL, vch, TO_CHAR);
            act("Bands of dark power loop around $N, binding $M to $n!", ch, NULL, vch, TO_ROOM);

            ch->pet = vch;
            vch->master = ch;
            vch->leader = ch;
            return true;
        }

        check_improve(ch, vch, gsn_reshackle, false, 2);
    }

    stop_follower(vch);

    if (IS_NPC(vch) && !vch->demontrack)
    {
    	int chance = 0;
    	if (IS_NEUTRAL(mch))	chance += 5;
    	if (IS_EVIL(mch))	chance += 10;
    	if (IS_NEUTRAL(vch))	chance += 5;
    	if (IS_EVIL(vch))	chance += 10;

    	if (number_percent() < chance)
    	{
            act("$n screams in fury and attacks $N!", vch, NULL, mch, TO_NOTVICT);
            act("$n screams in fury and attacks you!", vch, NULL, mch, TO_VICT);
            multi_hit(vch, mch, TYPE_UNDEFINED);
        }
    }

    return TRUE;
}

bool spell_vengeance (int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    int dam = 0;

    if (victim == ch)
    {
	send_to_char("You call the wrath of the spirit world upon yourself!\n\r", ch);
	act("$n calls the wrath of the spirit world upon $mself!", ch, NULL, NULL, TO_ROOM);
    }
    else
    {
        act("You call the wrath of the spirit world upon $N!", ch, NULL, victim, TO_CHAR);
        act("$n calls the wrath of the spirit world upon $N!", ch, NULL, victim, TO_NOTVICT);
        act("$n calls the wrath of the spirit world upon you!", ch, NULL, victim, TO_VICT);
    }

    if (IS_NPC(victim) || ((victim->pcdata->align_kills[ALIGN_GOOD] == 0) && IS_GOOD(victim)))
    {
	act("$N is unaffected by your call for vengeance!", ch, NULL, victim, TO_CHAR);
	act("$N is unaffected by $n's call for vengeance!", ch, NULL, victim, TO_NOTVICT);
	act("You are unaffected by $n's call for vengeance!", ch, NULL, victim, TO_VICT);
	return TRUE;
    }

    if (IS_NPC(victim))
    {
        if (IS_EVIL(victim))
	    dam = dice(level, 3);
    }
    else
        dam = (victim->pcdata->align_kills[ALIGN_GOOD] * 20);

    if (saves_spell(level, ch, victim, DAM_HOLY))
        dam /= 2;

    damage_old(ch, victim, dam, sn, DAM_HOLY, TRUE);

    return TRUE;
}

bool spell_visions (int sn, int level, CHAR_DATA *ch, void *vo,int target)
{
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
    CHAR_DATA *victim = (CHAR_DATA *) vo;

    if (is_affected(victim, sn))
    {
        act("$n is already tormented with visions of damnation.", ch, NULL, victim, TO_CHAR);
        return FALSE;
    }

    act("You call on the spiritworld to torment $N with visions of damnation!", ch, NULL, victim, TO_CHAR);
    act("$n calls on the spiritworld to torment you with visions of damnation!", ch, NULL, victim, TO_VICT);
    act("$n calls on the spiritworld to torment $N with visions of damnation!", ch, NULL, victim, TO_NOTVICT);

    if (victim == ch)
    {
        send_to_char("You know yourself too well to show visions to yourself.\n\r",ch);
        return FALSE;
    }

    if (is_affected(victim, gsn_mindshell) || is_affected(victim,gsn_mindshield))
    {
        act("$N is shielded against that attack", ch, NULL, victim, TO_CHAR);
        return FALSE;
    }

    if (saves_spell(level, ch, victim, DAM_MENTAL))
    {
	act("$N resists your attempt to force visions of damnation on $M.", ch, NULL, victim, TO_CHAR);
	act("You resist $n's attempt to force visions of damnation on you.", ch, NULL, victim, TO_VICT);
	act("$N resists $n's attempt to force visions of damnation on $M.", ch, NULL, victim, TO_NOTVICT);
	return TRUE;
    }

    if (IS_GOOD(victim))
        {
        act("$N stands firm in $S purity, and resists.", ch, NULL, victim, TO_CHAR);
        act("You stand firm in your purity, and resist.", ch, NULL, victim, TO_VICT);
        act("$N stands firm in $S purity, and resists.", ch, NULL, victim, TO_NOTVICT);
        return TRUE;
        }

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = level/8;
    af.location  = 0;
    af.modifier  = 0;
    af.bitvector = 0;
    affect_to_char( victim, &af );

    return TRUE;
}


bool spell_zeal(int sn, int level, CHAR_DATA *ch, void *vo,int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;

    if (is_affected(victim,sn) || IS_AFFECTED(victim,AFF_BERSERK))
    {
        if (victim == ch)
          send_to_char("You are already worked up.\n\r",ch);
        else
          act("$N is already worked up.",ch,NULL,victim,TO_CHAR);
        return FALSE;
    }

    if (is_affected(victim,skill_lookup("calm")) || IS_NAFFECTED(ch, AFF_SUBDUE))
    {
        if (victim == ch)
          send_to_char("You feel too level headed to be zealous.\n\r",ch);
        else
          act("$N looks too relaxed to be zealous.", ch,NULL,victim,TO_CHAR);
        return FALSE;
    }

    if (is_affected(victim, gsn_heartofstone))
    {
        if (victim == ch) send_to_char("Your stony heart cannot be stirred up into such passions.\n", ch);
        else act("Your magics fail to stir $N's passions.", ch, NULL, victim, TO_CHAR);
        return false;
    }

    if (is_affected(victim, gsn_unrealincursion))
    {
        if (victim == ch) send_to_char("Your reality is too distorted to be so sure of your cause.\n", victim);
        else act("$N's reality is too distorted to maintain any sort of zealous strength.", ch, NULL, victim, TO_CHAR);
        return false;
    }

    int mod(level / 5);
    if (perform_litany_skill_check(ch, NULL, Litany_Benediction, 1))
        mod = (mod * 13) / 10;

    AFFECT_DATA af = {0};
    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = (level * (check_durablemagics(*ch) ? 2 : 1)) / 4;
    af.modifier  = mod;

    af.location  = APPLY_HITROLL;
    affect_to_char(victim,&af);

    af.location  = APPLY_DAMROLL;
    affect_to_char(victim,&af);

    af.modifier  = 10 * (level / 12);
    af.location  = APPLY_AC;
    affect_to_char(victim,&af);

    send_to_char("You grow angry and feel a zealous strength fill you.\n\r",victim);
    act("$n gets a zealous look in $s eyes!",victim,NULL,NULL,TO_ROOM);

    return TRUE;
}

bool check_spirit_of_freedom(CHAR_DATA *ch)
{
    AFFECT_DATA *paf;

    for (paf = ch->affected; paf; paf = paf->next)
	if (paf->type == gsn_spirit_of_freedom)
	    if (number_percent() < paf->modifier)
		return TRUE;

    return FALSE;
}

bool spell_glowing_gaze(int sn,int level,CHAR_DATA *ch,void *vo,int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    static const int dam_each[] =
    {
         0,
         5,  5,  10,  12,  14,   16, 20, 25, 29, 33,
        36, 39, 39, 39, 40,     40, 41, 41, 42, 42,
        43, 43, 44, 44, 45,     45, 46, 46, 47, 47,
        48, 48, 49, 49, 50,     50, 51, 51, 52, 52,
        53, 53, 54, 54, 55,     55, 56, 56, 57, 57
    };
    int dam;

    level       = UMIN(level, static_cast<int>(sizeof(dam_each)/sizeof(dam_each[0])) - 1);
    level       = UMAX(0, level);
    dam         = number_range( dam_each[level] / 2, dam_each[level] * 2 );
    if ( saves_spell( level, ch, victim,DAM_LIGHT) )
        dam /= 2;
    damage_old( ch, victim, dam, sn, DAM_LIGHT ,TRUE);
    return TRUE;
}

bool spell_seraphic_wings( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if (IS_AFFECTED(victim, AFF_FLYING) || is_affected(victim, sn))
    {
        if (victim == ch)
            send_to_char("You are already magically imbued with flight.\n\r",ch);
        else
            act("$N doesn't need your help to fly.", ch, NULL, victim, TO_CHAR);

        return FALSE;
    }

    if (!victim->in_room)
        return FALSE;

    if (aura_grade(victim) > 0)
    {
	send_to_char("You are too tainted to don seraphic wings.\n\r",ch);
	return FALSE;
    }
    
    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = level/2;
    af.location  = 0;
    af.modifier  = 0;
    af.bitvector = AFF_FLYING;
    affect_to_char( victim, &af );
    send_to_char( "Seraphic wings billow out from your back, lifting you off the ground.\n\r", victim );
    act( "Seraphic wings billow out from $n's back, lifting $m off the ground.", victim, NULL, NULL, TO_ROOM );
    return TRUE;
}

bool spell_lesserspiritshield(int sn,int level,CHAR_DATA *ch,void *vo, int target)
{
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if (IS_TRUE_EVIL(ch))
    {
	send_to_char("The spirit shield has no effect on you.\n\r",ch);
	return FALSE;
    }
    if (is_affected(ch,gsn_lesserspiritshield) || is_affected(ch,gsn_spiritshield))
    {
	send_to_char("You are already surrounded by a spirit shield.\n\r",ch);
	return FALSE;
    }
    act("A warm glow surrounds you.",ch,NULL,NULL,TO_CHAR);
    act("A warm glow surrounds $n.",ch,NULL,NULL,TO_ROOM);

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = 24;
    af.location  = 0;
    af.modifier  = 0;
    af.bitvector = 0;
    affect_to_char(ch, &af);
			
    return TRUE;
}

bool spell_remove_hex( int sn, int level, CHAR_DATA *ch, void *vo,int target)
{

    send_to_char("A soft white glow surrounds you.\n\r", ch);
    act("A soft white glow surrounds $n.", ch, NULL, NULL, TO_ROOM);

    if (check_dispel(level,ch,gsn_curse))
        act("$n looks more relaxed.",ch,NULL,NULL,TO_ROOM);
    if (check_dispel(level,ch,gsn_embraceisetaton))
	act("The dark tentacles around $n turn a pale grey, and crumble into dust.",ch,NULL,NULL,TO_ROOM);
    if (check_dispel(level,ch,gsn_blight))
	act("$n looks less helpless.",ch,NULL,NULL,TO_ROOM);
    if (check_dispel(level,ch,gsn_nightfears))
	act("$n's nightmares subside.",ch,NULL,NULL,TO_ROOM);
    
    return TRUE;
}

bool spell_invocationofconveru( int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    // Check for cooldown
    if (is_affected(ch, sn))
    {
        send_to_char("You are not yet ready to again call upon Converu's legacy.\n", ch);
        return false;
    }

    // Apply the effect
    send_to_char("The Spirit of the Warrior fills you, speeding your movements!\n", ch);
    AFFECT_DATA af = {0};
    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = 3;
    af.bitvector = AFF_HASTE;
    affect_to_char(ch, &af);

    // Also add a cooldown
    af.duration  = 13;
    af.bitvector = 0;
    affect_to_char(ch, &af);

    return true;
}
