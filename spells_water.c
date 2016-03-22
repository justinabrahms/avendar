#if defined(macintosh)
#include <types.h>
#else
#include <sys/types.h>
#endif
#include <stdio.h>
#include <time.h>
#include <sstream>
#include <set>
#include "spells_water.h"
#include "magic.h"
#include "skills_chirurgeon.h"
#include "spells_fire.h"
#include "tables.h"
#include "spells_spirit.h"
#include "RoomPath.h"
#include "Direction.h"
#include "EchoAffect.h"
#include "spells_void.h"

/* External declarations */
DECLARE_DO_FUN(do_look);
DECLARE_DO_FUN(do_autoyell);
DECLARE_DO_FUN(do_yell);

extern void weather_update (void);
extern void show_char_status( CHAR_DATA *victim, CHAR_DATA *ch);
extern	bool	global_bool_ranged_attack;

bool check_group_fieldmedicine_save(int sn, int diseaseLevel, CHAR_DATA * ch)
{
    if (ch->in_room == NULL)
        return false;

    for (CHAR_DATA * gch(ch->in_room->people); gch != NULL; gch = gch->next_in_room)
    {
        if ((gch == ch || is_same_group(ch, gch)) && number_percent() <= get_skill(gch, gsn_fieldmedicine))
        {
            check_improve(gch, NULL, gsn_fieldmedicine, true, 4);
            if (saves_spell(diseaseLevel, NULL, ch, DAM_DISEASE))
            {
                std::ostringstream mess;
                mess << "$N's ministrations stave off the effects of your " << skill_table[sn].name << ", for the moment.";
                act(mess.str().c_str(), ch, NULL, gch, TO_CHAR);

                mess.str("");
                mess << "You tend to $N's " << skill_table[sn].name << ", staving off its effects for the moment.";
                act(mess.str().c_str(), gch, NULL, ch, TO_CHAR);

                return true;
            }
        }
    }

    return false;
}

bool is_water_room(const ROOM_INDEX_DATA & room)
{
    // Check sector type
    switch (room.sector_type)
    {
        case SECT_UNDERWATER:
        case SECT_WATER_SWIM:
        case SECT_WATER_NOSWIM: return true;
    }

    return false;
}

std::string checkDiagnoseTags(CHAR_DATA * ch, CHAR_DATA * victim)
{
    // Perform skill check
    if (number_percent() > get_skill(ch, gsn_diagnose))
        return "";

    // Skill check passed, look for maladies visible to all water scholars
    std::ostringstream mess;
    if (is_affected(victim, gsn_poison)) mess << "(Poisoned) ";
    if (is_affected(victim, gsn_blindness)) mess << "(Blind) ";
    
    if (is_affected(victim, gsn_pox)) mess << "(Poxed) ";
    if (is_affected(victim, gsn_fever)) mess << "(Fevered) ";
    if (is_affected(victim, gsn_plague)) mess << "(Plagued) ";

    if (is_affected(victim, gsn_enfeeblement)) mess << "(Feeble) ";
    if (is_affected(victim, gsn_heatwave)) mess << "(Overheated) ";
    if (is_affected(victim, gsn_weaken)) mess << "(Weak) ";
    if (is_affected(victim, gsn_weariness)) mess << "(Weary) ";
    if (is_affected(victim, gsn_enervatingray)) mess << "(Enervated) ";
    
    // Check for dousable maladies
    if (is_affected(victim, gsn_consume)) mess << "(Consumed) ";
    if (is_affected(victim, gsn_ignite)) mess << "(Ignited) ";
    if (is_affected(victim, gsn_aggravatewounds)) mess <<"(Aggravated Wounds) ";
    if (is_affected(victim, gsn_smolder)) mess << "(Smoldering) ";

    // Check for mendable wounds
    if (is_affected(victim, gsn_gouge)) mess << "(Gouged) ";
    if (is_affected(victim, gsn_hamstring)) mess << "(Hamstrung) ";
    if (is_affected(victim, gsn_slice)) mess << "(Sliced) ";
    if (is_affected(victim, gsn_impale)) mess << "(Impaled) ";
    if (is_affected(victim, gsn_vitalstrike)) mess << "(Gut Wound) ";
    if (is_affected(victim, gsn_cleave)) mess << "(Cloven) ";
    if (is_affected(victim, gsn_gash)) mess << "(Gashed) ";
    if (is_affected(victim, gsn_boneshatter)) mess << "(Boneshattered) ";
    if (is_affected(victim, gsn_kneeshatter)) mess << "(Kneeshattered) ";

    // Check for conditions which don't fit into neat categories
    if (is_affected(victim, gsn_curse)) mess << "(Cursed) ";
    if (!IS_NPC(victim) && victim->pcdata->condition[COND_DRUNK] > 10) mess << "(Drunk) ";

    // Check for maladies only visible to those with clarify mind
    if (number_percent() <= get_skill(ch, gsn_clarifymind))
    {
        if (is_affected(victim, gsn_delusions)) mess << "(Deluded) ";
        if (is_affected(victim, gsn_obfuscation)) mess << "(Obfuscated Sight) ";
        if (is_affected(victim, gsn_nightfears)) mess << "(Nightmares) ";
        if (is_affected(victim, gsn_powerwordfear)) mess << "(Afraid) ";
        if (is_affected(victim, gsn_visions)) mess << "(Tormented) ";
        if (is_affected(victim, gsn_seedofmadness)) mess << "(Mad) ";
        if (!find_cloakers(victim).empty()) mess << "(Cloaked Sight) ";
    }

    // Check for improvement
    std::string result(mess.str());
    if (!result.empty())
        check_improve(ch, NULL, gsn_diagnose, true, 12);

    return result;
}

void checkPhysickersInstinct(CHAR_DATA * ch, int & healAmount)
{
    if (!is_affected(ch, gsn_physikersinstinct))
        return;

    int bonus(12 + UMAX(0, (get_skill(ch, gsn_physikersinstinct) - 60) / 5));
    healAmount = (healAmount * (100 + bonus)) / 100;
}

static void checkRevenant(CHAR_DATA & victim, int & healAmount)
{
    if (number_percent() <= get_skill(&victim, gsn_revenant))
        healAmount = (healAmount * 3) / 4;
}

bool checkLogorPreventHealing(CHAR_DATA * ch, CHAR_DATA * victim)
{
    // Check for affect existence
    AFFECT_DATA * logorAff(get_affect(victim, gsn_ashesoflogor));
    if (logorAff == NULL)
        return false;

    // Check for save
    if (saves_spell(logorAff->level, NULL, victim, DAM_FIRE))
        return false;

    // Failed save
    send_to_char("Even as the healing magic starts to work, you can feel the defiling ashes within you burn away its power.\n", victim);
    send_to_char("Your healing magic has no effect!\n", ch);
    return true;
}

bool checkMartyrsFire(CHAR_DATA * ch, CHAR_DATA * victim, int & healAmount)
{
    // Check for affect existence
    AFFECT_DATA * martyrAff(get_affect(ch, gsn_martyrsfire));
    if (martyrAff == NULL)
        return false;

    // Martyr affect is up, check for casting on oneself
    if (ch == victim)
        send_to_char("You burn away some of your life, but cannot manage to channel the extra energy into yourself!\n", ch);
    else if (is_affected(victim, gsn_martyrsfire))
        act("You burn away some of your life, but the martyr's aura around $N prevents you from empowering your healing!", ch, NULL, victim, TO_CHAR);
    else
    {
        send_to_char("You burn away some of your own life to empower the healing magic.\n", ch);
        int healPercent((martyrAff->level * 2) + UMAX(0, (martyrAff->modifier - 75)));
        healAmount += dice(10, UMAX(1, (healAmount * healPercent) / 500));
    }

    // Hurt the caster
    int dam(dice(6, 2 * martyrAff->level) - dice(6, 2 * martyrAff->level));
    if (dam <= 1) dam = dice(2, 8);
    ch->hit -= dam;

    // Check for death
    update_pos(ch);
    if (ch->position == POS_DEAD)
    {
        act("$n trembles, having spent too much of $s own life to heal $N.", ch, NULL, victim, TO_NOTVICT);
        act("$n trembles, having spent too much of $s own life to heal you.", ch, NULL, victim, TO_VICT);
        act("$n collapses as blue flames lick at $s flesh! The fire spreads quickly, consuming $m.", ch, NULL, NULL, TO_ROOM);
        send_to_char("You tremble, having spent too much of your own life to empower your healing.\n", ch);
        send_to_char("You collapse as blue flames lick at your flesh! The fire spreads quickly, consuming you!\n", ch);
        std::ostringstream mess;
        mess << ch->name << " died a martyr to save " << victim->name << " in " << ch->in_room->name << " [" << ch->in_room->vnum << "]";
        wiznet(const_cast<char*>(mess.str().c_str()), NULL, NULL, WIZ_DEATHS, 0, 0);
        log_string(mess.str().c_str());
        raw_kill(ch);
    }

    return true;
}

void do_somaticarts(CHAR_DATA * ch, char * argument)
{
    // Check for skill
    if (!IS_IMMORTAL(ch) && get_skill(ch, gsn_somaticarts) <= 0)
    {
        send_to_char("Huh?\n", ch);
        return;
    }

    // Check for alternate target for imms
    CHAR_DATA * victim(ch);
    if (IS_IMMORTAL(ch) && argument[0] != '\0')
    {
        victim = get_char_world(ch, argument);
        if (victim == NULL)
        {
            send_to_char("You don't see them in the world.\n", ch);
            return;
        }
    }

    if (IS_NPC(victim))
    {
        send_to_char("Only adventurers may learn the somatic arts.\n", ch);
        return;
    }

    // Send the results
    std::string listing;
    if (victim->pcdata->somatic_arts_info == NULL) listing = "None\n";
    else listing = victim->pcdata->somatic_arts_info->Display();

    if (victim == ch) send_to_char("You are versed in the following somatic arts:\n", ch);
    else act("$N is versed in the following somatic arts:", ch, NULL, victim, TO_CHAR);
    send_to_char(listing.c_str(), ch);
}

bool spell_glyphofulyon(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    // Sanity-check
    if (ch->in_room == NULL)
    {
        bug("glyph of ulyon called from NULL room", 0);
        return false;
    }

    // Check for cooldown
    if (is_affected(ch, sn))
    {
        send_to_char("You are not prepared to scribe another glyph of Ulyon yet.\n", ch);
        return false;
    }

    // Check for glyph already present
    if (room_is_affected(ch->in_room, sn))
    {
        send_to_char("Such a glyph has already been scribed here.\n", ch);
        return false;
    }

    // Check for ground
    if (!ON_GROUND(ch))
    {
        send_to_char("You may only inscribe such a glyph on solid ground.\n", ch);
        return false;
    }

    // Echoes
    act("You hunch, tracing the glyph of Ulyon on the ground and imbuing it with your will.", ch, NULL, NULL, TO_CHAR);
    act("$n hunches over, tracing a simple glyph on the ground.", ch, NULL, NULL, TO_ROOM);
    act("After a moment's pause, the glyph begins to gleam a soft blue, tinging the air with invigorating energy.", ch, NULL, NULL, TO_ALL);

    // Apply the effect
    AFFECT_DATA af = {0};
    af.where    = TO_ROOM;
    af.level    = level;
    af.type     = sn;
    af.duration = (level / 10);
    affect_to_room(ch->in_room, &af);

    // Apply a cooldown
    af.where    = TO_AFFECTS;
    af.duration = 28 - UMAX(0, (get_skill(ch, sn) - 70) / 5);
    affect_to_char(ch, &af);
    return true;
}

bool spell_solaceoftheseas(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    CHAR_DATA * victim(static_cast<CHAR_DATA*>(vo));

    // Check for cooldowns on both caster and target
    if (is_affected(ch, sn))
    {
        send_to_char("You cannot call upon the solace of the seas for some time yet.\n", ch);
        return false;
    }

    if (is_affected(victim, sn))
    {
        act("$N cannot be wrapped in the cool embrace of the seas again for some time yet.", ch, NULL, victim, TO_CHAR);
        return false;
    }

    // Verify groupmate
    if (ch != victim && !is_same_group(ch, victim))
    {
        send_to_char("Only a groupmate would stay quiet long enough to hear your call to the seas.\n", ch);
        return false;
    }

    // Echoes
    act("You close your eyes and whisper, beckoning to the heart of the ocean itself.", ch, NULL, NULL, TO_CHAR);
    send_to_char("A sense of serenity engulfs you, calming your senses and slowing your pulse.\n", victim);

    if (ch == victim) 
        act("$n closes $s eyes and whispers, and suddenly the space about $m seems calm and muted.", ch, NULL, NULL, TO_ROOM);
    else
    {
        act("The space about $N seems to grow calm in muted as the solace of the seas washes over $M.", ch, NULL, victim, TO_CHAR);
        act("$n closes $s eyes and whispers, and suddenly the space about $N seems calm and muted.", ch, NULL, victim, TO_NOTVICT);
    }

    // Apply the effect
    AFFECT_DATA af = {0};
    af.where    = TO_AFFECTS;
    af.type     = sn;
    af.level    = level;
    af.duration = 1;
    af.modifier = 1;
    af.bitvector = AFF_SLOW;
    affect_to_char(victim, &af);

    // Apply a cooldown to the victim
    af.modifier = 0;
    af.duration = 30 - UMAX(0, (get_skill(ch, sn) - 70) / 5);
    affect_to_char(victim, &af);

    // Apply a cooldown to the caster
    if (ch != victim)
    {
        af.bitvector = 0;
        affect_to_char(ch, &af);
    }

    return true;
}

bool spell_refinepotion(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    // Check for effect on the potion
    OBJ_DATA * obj(static_cast<OBJ_DATA*>(vo));
    if (obj_is_affected(obj, sn))
    {
        act("$p has already been refined.", ch, obj, NULL, TO_CHAR);
        return false;
    }

    // Check that this is, in fact, a potion
    if (obj->item_type != ITEM_POTION)
    {
        act("$p is not a potion.", ch, obj, NULL, TO_CHAR);
        return false;
    }

    // Improve the potion
    act("You hold $p tightly and chant, letting your magics enfold and refine it.", ch, obj, NULL, TO_CHAR);
    act("As $n chants over $p, a thin sheen of frost forms on it.", ch, obj, NULL, TO_ROOM);

    int maxBoost(3);
    maxBoost += UMAX(0, (level - 35) / 4);
    maxBoost += UMAX(0, (get_skill(ch, sn) - 70) / 10);
    obj->value[0] += number_range(2, maxBoost);

    // Apply effect
    AFFECT_DATA af = {0};
    af.where    = TO_OBJECT;
    af.type     = sn;
    af.level    = level;
    af.location = APPLY_HIDE;
    af.duration = -1;
    affect_to_obj(obj, &af);

    return true;
}

bool spell_wellspring(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    // Sanity-check
    if (ch->in_room == NULL)
    {
        bug("Wellspring called from NULL room", 0);
        return false;
    }

    // Verify that this place has ground
    if (!ON_GROUND(ch))
    {
        send_to_char("You cannot create a wellspring here.\n", ch);
        return false;
    }

    // Verify that there is not already a wellspring here
    for (OBJ_DATA * spring(ch->in_room->contents); spring != NULL; spring = spring->next_content)
    {
        if (spring->pIndexData->vnum == OBJ_VNUM_WELLSPRING)
        {
            send_to_char("A wellspring already flows here.\n", ch);
            return false;
        }
    }
    
    // Create the spring
    OBJ_DATA * spring(create_object(get_obj_index(OBJ_VNUM_WELLSPRING), 0));
    spring->level = level;
    spring->timer = level / 3;
    if (ch->in_room && room_is_affected(ch->in_room, gsn_poisonspirit))
        spring->value[3] = 1;

    obj_to_room(spring, ch->in_room);
    act("$p flows from the ground, sparkling with clear, pure energy.", ch, spring, NULL, TO_CHAR);
    act("$p flows from the ground, sparkling with clear, pure energy.", ch, spring, NULL, TO_ROOM);
    return true;
}

bool spell_frostblast(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;

    int diceSize(4);
    int bonus(10);
    AFFECT_DATA * arctic(get_affect(ch, gsn_arcticchill));
    if (arctic != NULL && arctic->modifier == 0)
    {
        diceSize = 5;
        bonus = 20;
    }

    int dam(dice(level, diceSize) + bonus);
    if (saves_spell(level, ch, victim, DAM_COLD))
        dam /= 2;

    act ("$n unleashes a blast of frost upon $N!", ch, NULL, victim, TO_NOTVICT);
    act ("You unleash a blast of frost upon $N!", ch, NULL, victim, TO_CHAR);
    act ("$n unleashes a blast of frost upon you!", ch, NULL, victim, TO_VICT);
    damage_old(ch, victim, dam, sn, DAM_COLD, true);
    return true;
}

bool spell_markofthekaceajka(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    // Sanity-check
    if (ch->in_room == NULL)
    {
        bug("Mark of the kaceajka called from NULL room", 0);
        return false;
    }

    // Check for room effect
    if (room_is_affected(ch->in_room, sn))
    {
        send_to_char("This place already has such a mark.\n", ch);
        return false;
    }

    // Check for cooldown
    if (is_affected(ch, sn))
    {
        send_to_char("You are not ready to place another mark of the kaceajka.\n", ch);
        return false;
    }

    // Check for grounded
    if (!ON_GROUND(ch))
    {
        send_to_char("There is no solid ground on which to inscribe such a mark.\n", ch);
        return false;
    }

    // Spell is good to go
    struct CallbackHandler
    {
        static bool HandleMoveFrom(CHAR_DATA * ch, ROOM_INDEX_DATA *, EchoAffect *, void *)
        {
            send_to_char("You abandon the rune, letting the magic dissipate.\n", ch);
            return true;
        }

        static bool HandlePositionChange(CHAR_DATA * ch, int newPos, EchoAffect *, void *)
        {
            if (newPos == POS_STANDING)
                return false;
            
            send_to_char("You abandon the rune, letting the magic dissipate.\n", ch);
            return true;
        }

        static bool FinishSpell(CHAR_DATA * ch, EchoAffect *, void * tag)
        {
            // Apply the effect to the room
            int level(reinterpret_cast<int>(tag));
            AFFECT_DATA af = {0};
            af.where    = TO_ROOM_AFF;
            af.type     = gsn_markofthekaceajka;
            af.level    = level;
            af.duration = level / 6;
            affect_to_room(ch->in_room, &af);

            // Add a cooldown
            af.where    = TO_AFFECTS;
            af.duration = 20 - UMAX(0, (get_skill(ch, gsn_markofthekaceajka) - 70) / 10);
            affect_to_char(ch, &af);
            return false;
        }
    };

    // Echo the start of the spell
    act("You hunch, tracing the lines of a simple rune on the ground.", ch, NULL, NULL, TO_CHAR);
    act("$n hunches, tracing the lines of a simple rune on the ground.", ch, NULL, NULL, TO_ROOM);

    // Set up the rest of the spell
    EchoAffect * echoAff(new EchoAffect(1));
    echoAff->SetTag(reinterpret_cast<void*>(level));
    echoAff->SetMoveCallback(&CallbackHandler::HandleMoveFrom);
    echoAff->SetPositionCallback(&CallbackHandler::HandlePositionChange);
    echoAff->AddLine(&CallbackHandler::FinishSpell,
                    "You breathe softly on the rune to finish it, and it glows a chill blue in response.",
                    "$n breathes softly on the rune, and it glows a chill blue in response.");
    EchoAffect::ApplyToChar(ch, echoAff);
    return true;
}

bool spell_draughtoftheseas(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    // Check for cooldown
    if (is_affected(ch, sn))
    {
        send_to_char("You are not yet prepared to create another draught of the seas.\n", ch);
        return false;
    }

    // Check for cooldown on the object
    OBJ_DATA * obj(static_cast<OBJ_DATA*>(vo));
    if (obj_is_affected(obj, sn))
    {
        act("The waters of $p have already been blessed as a draught of the seas.", ch, obj, NULL, TO_CHAR);
        return false;
    }

    // Make sure this is a drink container with water in it
    if (obj->item_type != ITEM_DRINK_CON || obj->value[2] != liq_lookup("water"))
    {
        act("Only a container of pure water may be blessed as a draught of the seas.", ch, NULL, NULL, TO_CHAR);
        return false;
    }

    if (obj->value[1] <= 0)
    {
        act("There is nothing in $p to bless as a draught of the seas.", ch, obj, NULL, TO_CHAR);
        return false;
    }

    // Spell is going over
    act("You chant, and $p shines briefly with a pale blue light.", ch, obj, NULL, TO_CHAR);
    act("$n chants, and $p shines briefly with a pale blue light.", ch, obj, NULL, TO_ROOM);

    // Apply the effect
    AFFECT_DATA af = {0};
    af.where    = TO_OBJECT;
    af.type     = sn;
    af.level    = level;
    af.duration = 24;
    af.modifier = -1;
    affect_to_obj(obj, &af);

    af.where    = TO_AFFECTS;
    affect_to_char(ch, &af);

    return true;
}

bool spell_physikersinstinct(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    // Check for effect already present
    if (is_affected(ch, sn))
    {
        send_to_char("You are already focusing intently on healing!\n", ch);
        return false;
    }

    // Add the effect
    AFFECT_DATA af = {0};
    af.where    = TO_AFFECTS;
    af.type     = sn;
    af.level    = level;
    af.duration = 10;
    affect_to_char(ch, &af);

    send_to_char("You call upon the instincts of Salyra to guide your hands.\n", ch);
    return true;
}

bool spell_oceanswell(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    // Check for cooldown
    if (is_affected(ch, sn))
    {
        send_to_char("You are not yet ready to call forth the ocean's might again.\n", ch);
        return false;
    }

    // Check for water room
    if (ch->in_room == NULL || !is_water_room(*ch->in_room))
    {
        send_to_char("Calling upon the ocean's might requires more water than is present here.\n", ch);
        return false;
    }

    // Check for water elemental
    CHAR_DATA * elemental;
    for (elemental = ch->in_room->people; elemental != NULL; elemental = elemental->next_in_room)
    {
        if (IS_NPC(elemental) && elemental->pIndexData->vnum == MOB_VNUM_WATER_ELEMENTAL && elemental->master == ch && elemental->leader == ch)
            break;
    }

    if (elemental == NULL)
    {
        send_to_char("You have no water elemental here to imbue with the strength of the ocean.\n", ch);
        return false;
    }

    // Spell is good to go
    act("You hold a hand out towards $N, calling upon the power of the ocean to enfold $M.", ch, NULL, elemental, TO_CHAR);
    act("$n holds a hand out towards $N, calling out an arcane phrase.", ch, NULL, elemental, TO_ROOM);

    // Check for super success
    unsigned int count(1);
    if (number_percent() <= ((get_skill(ch, sn) - 80) / 4))
    {
        count = 2;
        act("With a heady rush of water and power, $N swells up, then splits into three!", ch, NULL, elemental, TO_ALL);
        act("As the watery energies slowly subsides, the thirds of $N fill out into full elementals, each larger than the original.", ch, NULL, elemental, TO_ALL);
    }
    else
    {
        act("With a rush of water and power, $N swells up, then splits into two!", ch, NULL, elemental, TO_ALL);
        act("As the watery energies slowly subsides, the halves of $N fill out into full elementals, each larger than the original.", ch, NULL, elemental, TO_ALL);
    }

    // Boost and heal the existing elemental
    ++elemental->level;
    elemental->damage[2] += (level / 5);
    elemental->hitroll += (level / 5);
    elemental->max_hit += (level * 5);
    elemental->hit = elemental->max_hit;

    // Apply a cooldown
    AFFECT_DATA af = {0};
    af.where    = TO_AFFECTS;
    af.type     = sn;
    af.level    = level;
    af.duration = (level / 2);
    affect_to_char(elemental, &af);

    // Make the copies 
    for (unsigned int i(0); i < count; ++i)
    {
        // Make a new elemental
        CHAR_DATA * newElemental(create_mobile(get_mob_index(MOB_VNUM_WATER_ELEMENTAL)));
        newElemental->level = elemental->level;
        newElemental->damroll = elemental->damroll;
        newElemental->hitroll = elemental->hitroll;
        newElemental->damage[0] = elemental->damage[0];
        newElemental->damage[1] = elemental->damage[1];
        newElemental->damage[2] = elemental->damage[2];
        newElemental->max_hit = elemental->max_hit;
        newElemental->hit = newElemental->max_hit;
        char_to_room(newElemental, ch->in_room);
        newElemental->master = ch;
        newElemental->leader = ch;
        affect_to_char(newElemental, &af);
    }
   
    // Give a cooldown to the caster, as well
    af.duration += (10 - UMAX(0, (get_skill(ch, sn) - 70) / 10));
    affect_to_char(ch, &af);
    return true;
}

std::set<ROOM_INDEX_DATA*> connected_water_rooms(ROOM_INDEX_DATA & startRoom)
{
    std::set<ROOM_INDEX_DATA*> rooms;
    
    // Check for water
    if (!is_water_room(startRoom))
        return rooms;

    // Get all connected water rooms
    std::set<ROOM_INDEX_DATA*> currRooms;
    currRooms.insert(&startRoom);
    
    while (!currRooms.empty())
    {
        // Move the first room in currRooms to rooms
        std::set<ROOM_INDEX_DATA*>::iterator iter(currRooms.begin());
        ROOM_INDEX_DATA * room(*iter);
        currRooms.erase(iter);
        rooms.insert(room);

        // Grab all the water connections
        for (unsigned int i(0); i < Direction::Max; ++i)
        {
            ROOM_INDEX_DATA * nextRoom(Direction::Adjacent(*room, static_cast<Direction::Value>(i)));
            if (nextRoom != NULL && rooms.find(nextRoom) == rooms.end() && is_water_room(*nextRoom))
                currRooms.insert(nextRoom);
        }
    }

    return rooms;
}

bool spell_maelstrom(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    // Sanity-check
    if (ch->in_room == NULL)
    {
        bug("Maelstrom cast from NULL room", 0);
        return false;
    }

    // Check for cooldown
    if (is_affected(ch, sn))
    {
        send_to_char("You are not yet prepared to call forth another maelstrom.\n", ch);
        return false;
    }

    // Verify sufficient rooms to support a maelstrom
    std::set<ROOM_INDEX_DATA*> rooms(connected_water_rooms(*ch->in_room));
    if (rooms.size() < 5)
    {
        if (rooms.empty()) send_to_char("You cannot call a maelstrom with no water!\n", ch);
        else send_to_char("There is not enough water about you to support a maelstrom.\n", ch);
        return false;
    }

    act("You hold your hands in the water, palms flat, and quietly infuse it with your will.", ch, NULL, NULL, TO_CHAR);
    act("$n holds $s hands in the water, palms flat, and quietly murmurs an arcane word.", ch, NULL, NULL, TO_ROOM);

    // Apply the maelstrom effect
    AFFECT_DATA af = {0};
    af.where    = TO_ROOM_AFF;
    af.type     = sn;
    af.level    = level;
    af.duration = 1 + (level / 9);

    for (std::set<ROOM_INDEX_DATA*>::const_iterator iter(rooms.begin()); iter != rooms.end(); ++iter)
    {
        affect_to_room(*iter, &af);
        if ((*iter)->people != NULL)
            act("The water all about you begins to churn angrily, swirling into the unmistakable pattern of a maelstrom!", (*iter)->people, NULL, NULL, TO_ALL);
    }

    // Apply a cooldown
    af.where    = TO_AFFECTS;
    af.duration = 120 - UMAX(0, (get_skill(ch, sn) - 80));
    affect_to_char(ch, &af);
    return true;
}

bool spell_drown(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    // Get the victim and check for the effect already being present
    CHAR_DATA * victim(static_cast<CHAR_DATA*>(vo));
    if (is_affected(victim, sn))
    {
        if (victim == ch) send_to_char("You are already drowning.\n", ch);
        else act("$N is already drowning.", ch, NULL, victim, TO_CHAR);
        return false;
    }

    // Check for a save
    int saveMod(0);
    if (ch->in_room != NULL)
    {
        switch (ch->in_room->sector_type)
        {
            case SECT_UNDERWATER: saveMod = 4; break;
            case SECT_WATER_SWIM:
            case SECT_WATER_NOSWIM: saveMod = 2; break;
        }
    }

    if (saves_spell(level + saveMod, ch, victim, DAM_DROWNING))
    {
        send_to_char("You feel your lungs start to fill up with water, but quickly cough it out.\n", victim);
        if (ch != victim)
            act("$N resists your attempts to fill $S lungs with water.", ch, NULL, victim, TO_CHAR);

        return true;
    }

    // Did not save
    if (is_affected(victim, gsn_waterbreathing))
        send_to_char("You feel your lungs fill up with water, but continue to breathe easily.\n", victim);
    else
        send_to_char("You feel your lungs fill up with water!\n", victim);

    if (ch != victim)
        act("Your magics take hold, and $N's lungs fill up with water!", ch, NULL, victim, TO_CHAR);

    AFFECT_DATA af = {0};
    af.where    = TO_AFFECTS;
    af.type     = sn;
    af.level    = level;
    af.duration = -1;
    affect_to_char(victim, &af);

    return true;
}

bool spell_arcticchill(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    // Check for cooldown
    if (is_affected(ch, sn))
    {
        send_to_char("You are not yet prepared to call further upon winter's power.\n", ch);
        return false;
    }

    // Echoes
    act("Your breath frosts over as a chill power fills you!", ch, NULL, NULL, TO_CHAR);
    act("$n's breath frosts over, and $e begins to emanate a palpable chill!", ch, NULL, NULL, TO_ROOM);

    // Apply the effect
    AFFECT_DATA af = {0};
    af.where    = TO_AFFECTS;
    af.type     = sn;
    af.level    = level;
    af.duration = (level / 5);
    affect_to_char(ch, &af);

    return true;
}

bool spell_breathofelanthemir(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    // Get the target and check for the effect already present
    CHAR_DATA * victim(static_cast<CHAR_DATA*>(vo));
    if (is_affected(victim, sn))
    {
        if (ch == victim) send_to_char("You are already chilled to your core.\n", ch);
        else act("$N is already chilled to $S core.", ch, NULL, victim, TO_CHAR);
        return false;
    }

    // Spell is casting; echo
    if (ch == victim)
    {
        act("You point a finger at yourself as you bark out a word of power.", ch, NULL, NULL, TO_CHAR);
        act("$n points a finger at $mself as $e barks out a word of power.", ch, NULL, NULL, TO_ROOM);
    }
    else
    {
        act("You point a finger at $N as you bark out a word of power.", ch, NULL, victim, TO_CHAR);
        act("$n points a finger at you as $e barks out a word of power.", ch, NULL, victim, TO_VICT);
        act("$n points a finger at $N as $e barks out a word of power.", ch, NULL, victim, TO_NOTVICT);
    }

    // Check for a save
    if (saves_spell(level, ch, victim, DAM_COLD))
    {
        send_to_char("You feel a icy grip seize you, but shake it off.\n", victim);
        return true;
    }

    // Did not save
    act("You feel your magic seize $N in an icy grip, chilling $M to the core!", ch, NULL, victim, TO_CHAR);
    act("You feel a icy grip seize you, chilling you to the core!", ch, NULL, victim, TO_VICT);
    act("$N's whole body turns a shade bluer as flecks of frost form on $S skin.", ch, NULL, victim, TO_NOTVICT);

    // Check for immolation
    if (is_affected(victim, gsn_immolation))
    {
        affect_strip(victim, gsn_immolation);
        send_to_char("The icy grip snuffs out the immolating fire within you!\n", victim);
    }

    // Apply the effect
    AFFECT_DATA af = {0};
    af.where    = TO_AFFECTS;
    af.type     = sn;
    af.level    = level;
    af.duration = (level / 3);
    affect_to_char(victim, &af);
    return true;
}

bool spell_glaciersedge(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    // Sanity-check
    if (ch->in_room == NULL)
    {
        bug("glacier's edge called from NULL room", 0);
        return false;
    }

    // Knock out invalid sector types
    switch (ch->in_room->sector_type)
    {
        case SECT_WATER_SWIM:
        case SECT_WATER_NOSWIM:
        case SECT_AIR:
            send_to_char("There is no solid ground here to mark as a glacier's edge.\n", ch);
            return false;
    }

    // Check for effect
    AFFECT_DATA * paf(get_affect(ch, sn));
    if (paf == NULL)
    {
        // First one placed
        AFFECT_DATA af = {0};
        af.where    = TO_AFFECTS;
        af.duration = -1;
        af.level    = level;
        af.type     = sn;
        af.location = APPLY_HIDE;
        af.point    = ch->in_room;
        affect_to_char(ch, &af);

        act("You lean down and place a palm flat against the ground, infusing it with glacial chill.", ch, NULL, NULL, TO_CHAR);
        act("$n leans down and places a palm flat against the ground, which frosts over in response.", ch, NULL, NULL, TO_ROOM);
        return true;
    }

    // Second one placed; try to draw a line between them
    RoomPath path(*ch->in_room, *(static_cast<ROOM_INDEX_DATA*>(paf->point)), ch);
    if (!path.Exists())
    {
        // No path found
        send_to_char("You place a palm flat against the ground, but can sense no connection to your previous marker.\n", ch);
        return false;
    }

    act("You lean down and place a palm flat against the ground, sensing the connection to your previous marker.", ch, NULL, NULL, TO_CHAR);
    act("$n leans down and places a palm flat against the ground, which frosts over in response.", ch, NULL, NULL, TO_ROOM);

    // Path found, apply effect to full line 
    AFFECT_DATA af = {0};
    af.where    = TO_ROOM_AFF;
    af.duration = 20;
    af.level    = level;
    af.type     = sn;
    af.modifier = ch->id;

    // Build the line
    ROOM_INDEX_DATA * room(ch->in_room);
    unsigned int i(0);
    while (true)
    {
        // Apply effect to room
        affect_to_room(room, &af);
        if (room->people != NULL)
            act("For a moment the air grows chill, making your breath come out as visible frost.", room->people, NULL, NULL, TO_ALL);

        // Check for end of the line
        if (i >= path.StepCount())
            break;

        // Find the next room
        room = Direction::Adjacent(*room, static_cast<Direction::Value>(path.StepDirection(i)));
        if (room == NULL)
        {
            // This should not happen
            bug("Got NULL room when following path in glacier's edge", 0);
            break;
        }

        ++i;
    } 

    affect_strip(ch, sn);
    return true;
}

bool spell_wintersstronghold(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    static const int MinRooms(10);
    static const int MaxRooms(20);
    
    // Sanity-check
    if (ch->in_room == NULL)
    {
        bug("winter's stronghold called from NULL room", 0);
        return false;
    }

    // Check for cooldown
    if (is_affected(ch, sn))
    {
        send_to_char("You are not yet prepared to call forth another stronghold of winter.\n", ch);
        return false;
    }

    // Determine room count
    int roomCount(number_range(MinRooms, MaxRooms));
    std::set<ROOM_INDEX_DATA*> rooms;
    std::vector<ROOM_INDEX_DATA*> roomBuffer0;
    std::vector<ROOM_INDEX_DATA*> roomBuffer1;
    std::vector<ROOM_INDEX_DATA*> * currRooms(&roomBuffer0);
    std::vector<ROOM_INDEX_DATA*> * nextRooms(&roomBuffer1);
    currRooms->push_back(ch->in_room);

    // Loop until there are no more rooms to process
    while (!currRooms->empty())
    {
        // Iterate the set of current rooms
        for (size_t i(0); i < currRooms->size(); ++i)
        {
            // Check whether this room has already been traversed
            ROOM_INDEX_DATA * room((*currRooms)[i]);
            if (rooms.find(room) != rooms.end())
                continue;

            // Not already traversed, so add it to the rooms list and bail out if the count is met
            rooms.insert(room);
            if (rooms.size() >= static_cast<unsigned int>(roomCount))
                break;

            // Add all adjacent rooms to the next round
            for (unsigned int j(0); j < Direction::Max; ++j)
            {
                ROOM_INDEX_DATA * nextRoom(Direction::Adjacent(*room, static_cast<Direction::Value>(j)));
                if (nextRoom != NULL)
                    nextRooms->push_back(nextRoom);
            }
        }

        // Reset for the next iteration
        std::swap(currRooms, nextRooms);
        nextRooms->clear();
    }

    // Check that the minimum number of rooms was hit
    if (rooms.size() < static_cast<unsigned int>(MinRooms))
    {
        send_to_char("This place is too enclosed to create a stronghold of winter.\n", ch);
        return false;
    }

    struct InnerFunctions
    {
        // Assumes both room and direction are valid
        static void AddEffect(ROOM_INDEX_DATA & room, int level, int duration, bool inside)
        {
            // Check for an effect already on the room
            AFFECT_DATA * paf(get_room_affect(&room, gsn_wintersstronghold));
            if (paf != NULL)
            {
                // Effect exists, update it
                paf->duration = UMAX(paf->duration, duration);
                if (inside) paf->modifier = 1;
                return;
            }

            // Add the effect
            AFFECT_DATA af = {0};
            af.where    = TO_ROOM_AFF;
            af.type     = gsn_wintersstronghold;
            af.level    = level;
            af.duration = duration;
            af.modifier = (inside ? 1 : 0);
            affect_to_room(&room, &af);
        }

        static void AddIceWall(ROOM_INDEX_DATA & room, Direction::Value direction)
        {
            if (!IS_SET(room.exit[direction]->exit_info, EX_ICEWALL))
            {
                // No ice wall in existence yet, so add one
                SET_BIT(room.exit[direction]->exit_info, EX_ICEWALL);
                if (room.people != NULL)
                {
                    std::ostringstream mess;
                    mess << "The air grows frosty as a thick sheet of ice rises " << Direction::DirectionalNameFor(direction) << ", sealing off the way.";
                    act(mess.str().c_str(), room.people, NULL, NULL, TO_ALL);
                }
            }
        }
    };

    // Echoes
    act("You bellow of word of power, and feel the temperature here drop noticeably in response!", ch, NULL, NULL, TO_CHAR);
    act("Thick walls of ice rise up a short distance away, forming this place into a fortress of winter itself!", ch, NULL, NULL, TO_CHAR);
    act("$n bellows a word of power, and the temperature here drops noticeably in response!", ch, NULL, NULL, TO_ROOM);

    // Add in walls of ice to every exit not connected to another one in the stronghold
    int duration(level / 12);
    for (std::set<ROOM_INDEX_DATA*>::const_iterator iter(rooms.begin()); iter != rooms.end(); ++iter)
    {
        ROOM_INDEX_DATA * room(*iter);
        std::vector<Direction::Value> dirs(Direction::ValidDirectionsFrom(*room));
        for (size_t i(0); i < dirs.size(); ++i)
        {
            // Get the adjacent room and check whether it is in the stronghold
            ROOM_INDEX_DATA * other(Direction::Adjacent(*room, dirs[i]));
            if (rooms.find(other) != rooms.end())
                continue;

            // Adjacent room is not in the stronghold, so wall it up
            InnerFunctions::AddIceWall(*room, dirs[i]);
            Direction::Value reverse(Direction::ReverseOf(dirs[i]));
            if (Direction::Adjacent(*other, reverse) == room)
            {
                // Add the wall to the adjacent room as well
                InnerFunctions::AddIceWall(*other, reverse);
                InnerFunctions::AddEffect(*other, level, duration, false);
            }
        }

        // Regardless of whether an ice wall was formed, make sure this room has the effect
        InnerFunctions::AddEffect(*room, level, duration, true);
    }

    // Add a cooldown
    AFFECT_DATA af = {0};
    af.where    = TO_AFFECTS;
    af.level    = level;
    af.type     = sn;
    af.duration = 120;
    affect_to_char(ch, &af);

    return true;
}

bool spell_wardoffrost(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    // Check for cooldown
    if (is_affected(ch, sn))
    {
        send_to_char("You are already warded by the frost.\n", ch);
        return false;
    }

    // Apply the effect
    AFFECT_DATA af = {0};
    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = 24;
    af.location  = APPLY_RESIST_FIRE;
    af.modifier  = (level / 2);
    af.bitvector = 0;
    affect_to_char(ch, &af);

    send_to_char( "You feel a cool aura form about you, quickly growing uncomfortably cold!\n", ch);
    act("A wintry chill begins to emanate from $n.", ch, NULL, NULL, TO_ROOM);
    return true;
}

bool spell_aquamove( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if (is_affected(victim, gsn_aquamove))
    {
	if (victim == ch)
	    send_to_char("You already feel able to move freely underwater.\n\r", ch);
	else
	    send_to_char("They are already able to move freely underwater.\n\r", ch);
        return FALSE;
    }

    af.where	 = TO_AFFECTS;
    af.type	 = sn;
    af.level	 = level;
    af.modifier  = 0;
    af.duration  = level;
    af.location  = 0;
    af.bitvector = 0;
    affect_to_char(victim, &af);

    send_to_char("You feel able to move freely underwater.\n\r", victim);
    if (victim != ch)
	act("You allow $N to move freely underwater.", ch, NULL, victim, TO_CHAR);

    return TRUE;
}

bool spell_cleansefood( int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    OBJ_DATA *item;
    AFFECT_DATA *af;
    int type;

    item = (OBJ_DATA *) vo;

    if (item->item_type != ITEM_FOOD && item->item_type != ITEM_DRINK_CON)
    {
        send_to_char("You can only cleanse food and drink.\n\r", ch);
        return FALSE;
    }

    for (type = 0;poison_table[type].spell_fun;type++)
	if (obj_is_affected(item,*(poison_table[type].sn)))
	    break;
    
    if (!obj_is_affected(item, gsn_poison) && !poison_table[type].spell_fun && item->value[3] == 0)
    {
        send_to_char("That is not tainted with poison.\n\r", ch);
        return TRUE;
    }

    for (af = item->affected; af != NULL ; af = af->next)
    {
        if (af->type == gsn_poison)
        {
            affect_remove_obj(item, af);
            item->value[3] = 0;
        }
	if (af->type == *(poison_table[type].sn))
	{
	    affect_remove_obj(item,af);
	    item->value[3] = 0;
	}
    }

    send_to_char("You feel the taint expiate.\n\r", ch);

    return TRUE;

}

bool spell_communion(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    // Sanity-check
    if (ch->in_room == NULL)
    {
        bug("Communion called from NULL room", 0);
        return false;
    }

    // Determine the groupmates in the room
    std::vector<CHAR_DATA*> groupmates;
    for (CHAR_DATA * gch(ch->in_room->people); gch != NULL; gch = gch->next_in_room)
    {
        if (ch == gch || is_same_group(ch, gch))
            groupmates.push_back(gch);
    }

    // Build the effect based on how many people are in the group
    AFFECT_DATA af = {0};
    af.where    = TO_AFFECTS;
    af.type     = sn;
    af.level    = level;
    af.duration = level;

    int effectiveLevel(level + (groupmates.size() - 1) * 5);
    act("You bow your head in reverence, murmuring softly as pure water slowly fills your cupped hands.", ch, NULL, NULL, TO_CHAR);
    act("$n bows $s head in reverence, murmuring softly as pure water slowly fills $s cupped hands.", ch, NULL, NULL, TO_ROOM);
    
    // Now apply the effect to all the groupmates
    for (size_t i(0); i < groupmates.size(); ++i)
    {
        // Strip any existing communion; if present, don't allow healing on this communion
        affect_strip(groupmates[i], gsn_communion);

        // Echoes
        if (ch == groupmates[i])
        {
            act("You gently sip from the holy water.", ch, NULL, NULL, TO_CHAR);
            act("$n sips gently from the holy water.", ch, NULL, NULL, TO_ROOM);
        }
        else
        {
            act("You gently sip from the holy water $N offers to you, and feel rejuvenated!", groupmates[i], NULL, ch, TO_CHAR);
            act("$n sips gently of the holy water offered by you.", groupmates[i], NULL, ch, TO_VICT);
            act("$n sips gently of the holy water offered by $N.", groupmates[i], NULL, ch, TO_NOTVICT);
        }

        // Apply the effect
        af.location = APPLY_SAVES;
        af.modifier = -effectiveLevel / 12;
        affect_to_char(groupmates[i], &af);

        af.location = APPLY_AC;
        af.modifier = -effectiveLevel;
        affect_to_char(groupmates[i], &af);
 
        af.location = APPLY_HIT;
        af.modifier = effectiveLevel / 2;
        affect_to_char(groupmates[i], &af);

        // Heal for the amount of boosted max hp
        groupmates[i]->hit = UMIN(groupmates[i]->max_hit, groupmates[i]->hit + af.modifier);
   }

    return true;
}

bool spell_coneofcold( int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    CHAR_DATA *vch, *vch_next;
    char buf[MAX_STRING_LENGTH];
    int dam;
   
    if (!ch->in_room)
	return FALSE;
    if (!victim)
	victim = ch->fighting;
    if (!victim)
        return FALSE;
    act("You unleash a cone of cold towards $N!", ch, NULL, victim, TO_CHAR);
    act("$n unleashes a cone of cold towards $N!", ch, NULL, victim, TO_NOTVICT);
    act("$n unleashes a cone of cold towards you!", ch, NULL, victim, TO_VICT);

    dam = dice(level, 5) - 20;
    dam = UMAX(dam, 10);

    if (saves_spell(level, ch, victim, DAM_COLD))
	damage_old(ch, victim, dam / 2, sn, DAM_COLD, TRUE);
    else
	damage_old(ch, victim, dam, sn, DAM_COLD, TRUE);

    for (vch = ch->in_room->people; vch; vch = vch_next)
    {
        vch_next = vch->next_in_room;
        if (vch->fighting == ch && vch != victim)
        {
            if (!IS_NPC(vch) && (vch != victim) 
            && (!ch->fighting || !vch->fighting))
            {
            if (can_see(vch, ch))
                sprintf(buf, "Help!  %s is attacking me!", PERS(ch, vch));
            else
                sprintf(buf, "Help!  Someone is attacking me!");
            do_autoyell(vch, buf);
            }
            check_killer(ch, vch);

            if (saves_spell(level, ch, vch, DAM_COLD))
                damage_old(ch, vch, dam / 2, sn, DAM_COLD, TRUE);
            else
                damage_old(ch, vch, dam, sn, DAM_COLD, TRUE);
        }
    }

    return TRUE;
}


bool spell_create_food( int sn, int level, CHAR_DATA *ch, void *vo,int target)
{
    OBJ_DATA *mushroom = NULL;
    char arg[MAX_STRING_LENGTH];
    int i;
    int index = number_range(OBJ_VNUM_FOODSTART, OBJ_VNUM_FOODSTART + (int)(ch->level * (OBJ_VNUM_FOODCOUNT-1)/LEVEL_HERO));
    index = URANGE(OBJ_VNUM_FOODSTART, index, OBJ_VNUM_FOODSTART + OBJ_VNUM_FOODCOUNT - 1);

    if (target_name[0] != '\0') //argument, scan to match short
    {
	    
	    target_name = one_argument(target_name, arg);
	    index = -1;
	    for (i = OBJ_VNUM_FOODSTART; i < OBJ_VNUM_FOODSTART + OBJ_VNUM_FOODCOUNT; i++)
	    {
		    if (!str_infix(arg, get_obj_index(i)->short_descr))
		    {
			    if (index < 0) index = i;
			    else
			    {
				    send_to_char("Multiple foods match that name. Try a different keyword.\n\r", ch);
				    return FALSE;
			    }
		    }
	    }
	    if (index < 0)
	    {
		    send_to_char("No foods match that name. Try a different keyword.\n\r", ch);
		    return FALSE;
	    }
    }

    if (number_percent() < 2 && ch->level < LEVEL_HERO) mushroom = create_object(get_obj_index(OBJ_VNUM_MUSHROOM), 0);
    else mushroom = create_object(get_obj_index(index), 0);
    
    mushroom->value[0] = 20 + level / 2;
    mushroom->value[1] = 30 + level;

    if (room_is_affected(ch->in_room, gsn_poisonspirit))
	mushroom->value[3] = 1;

    obj_to_room( mushroom, ch->in_room );
    act( "$p suddenly appears.", ch, mushroom, NULL, TO_ROOM );
    act( "$p suddenly appears.", ch, mushroom, NULL, TO_CHAR );
    return TRUE;
}

bool spell_create_water( int sn, int level, CHAR_DATA *ch, void *vo,int target)
{
/*    OBJ_DATA *obj = (OBJ_DATA *) vo;
    int water;

    if ( obj->item_type != ITEM_DRINK_CON )
    {
        send_to_char( "It is unable to hold water.\n\r", ch );
        return TRUE;
    }

    if ( obj->value[2] != LIQ_WATER && obj->value[1] != 0 )
    {
        send_to_char( "It contains some other liquid.\n\r", ch );
        return TRUE;
    }


    water = UMIN(
                level * (weather_info.sky >= SKY_RAINING ? 4 : 2),
                obj->value[0] - obj->value[1]
                );

    if ( water > 0 )
    {
        obj->value[2] = LIQ_WATER;
        obj->value[1] += water;
        if ( !is_name( "water", obj->name ) )
        {
            char buf[MAX_STRING_LENGTH];

            sprintf( buf, "%s water", obj->name );
            free_string( obj->name );
            obj->name = str_dup( buf );
        }
        act( "$p is filled.", ch, obj, NULL, TO_CHAR );
    }
*/
    return FALSE;
}

bool spell_cure_blindness(int sn,int level,CHAR_DATA *ch,void *vo,int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;

    if (victim->race == global_int_race_shuddeni)
    {
        send_to_char("There is no curing the everblind eyes of the shuddeni.\n\r", ch);
        return FALSE;
    }

    if ( !is_affected( victim, gsn_blindness ) 
      && !is_affected( victim, gsn_soulflare ) )
    {
        if (victim == ch)
          send_to_char("You aren't blind.\n\r",ch);
        else
          act("$N doesn't appear to be blinded.",ch,NULL,victim,TO_CHAR);
        return FALSE;
    }

    if (is_affected(victim,gsn_blindness))
    {
	if (check_dispel(level,victim,gsn_blindness))
	{
	    if (ch != victim)
            	act("$n is no longer blinded.",victim,NULL,NULL,TO_ROOM);
	}
	else
            if (victim == ch)
	    	act("You remain blinded.",ch,NULL,NULL,TO_CHAR);
	    else
		act("$N remains blinded.",ch,NULL,victim,TO_CHAR);

	return TRUE;
    }

    if (check_dispel(level,victim,gsn_soulflare))
	act("The brilliant light fades from $n's eyes, and $e can see again.",victim,NULL,NULL,TO_ROOM);
    else
	if (victim == ch)
	    act("You remain blinded.",ch,NULL,victim,TO_CHAR);
	else
	    act("$N remains blinded.",ch,NULL,victim,TO_CHAR);

    return TRUE;
}

bool spell_cure_critical( int sn, int level, CHAR_DATA *ch, void *vo,int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    int heal;

    if (checkLogorPreventHealing(ch, victim))
        return true;

    heal = dice(3, 8) + level - 6;
    checkMartyrsFire(ch, victim, heal);
    checkPhysickersInstinct(ch, heal);
    checkRevenant(*victim, heal);
    victim->hit = UMIN( victim->hit + heal, victim->max_hit );
    update_pos( victim );
    send_to_char( "You feel better!\n\r", victim );
    if ( ch != victim )
        if(IS_NPC(victim))
	    act("You cure $N's critical wounds.",ch,NULL,victim,TO_CHAR);
	else
	{
            send_to_char("After your healing, ",ch);
	    if(!is_affected(victim,gsn_berserk))
	        show_char_status(victim,ch);
	    else
	        act("some of $N's wounds close.",ch,NULL,victim,TO_CHAR);
        }	    
    return TRUE;
}

bool spell_cure_disease( int sn, int level, CHAR_DATA *ch,void *vo,int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;

    if ((is_affected( victim, gsn_fever))
     || (is_affected(victim, gsn_pox))
     || (is_affected(victim, gsn_plague)))
     {
        if (check_dispel(level,victim,gsn_fever)
         || check_dispel(level,victim,gsn_plague)
         || check_dispel(level,victim,gsn_pox))
        {
            send_to_char("You feel better.\n\r",victim);
            act("$n looks relieved as $s disease vanishes.",
            victim,NULL,NULL,TO_ROOM);
        }
        else
            send_to_char("Spell failed.\n\r",ch);

	return TRUE;
    }
    else
    {
        if (victim == ch)
          send_to_char("You aren't ill.\n\r",ch);
        else
          act("$N doesn't appear to be diseased.",ch,NULL,victim,TO_CHAR);
        return FALSE;
    }
}


bool spell_cure_light( int sn, int level, CHAR_DATA *ch, void *vo,int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    int heal;

    if (checkLogorPreventHealing(ch, victim))
        return true;

    heal = dice(1, 8) + level / 3;
    checkMartyrsFire(ch, victim, heal);
    checkPhysickersInstinct(ch, heal);
    checkRevenant(*victim, heal);
    victim->hit = UMIN( victim->hit + heal, victim->max_hit );
    update_pos( victim );
    send_to_char( "You feel better!\n\r", victim );
    if ( ch != victim )
        if(IS_NPC(victim))
	    act("You cure $N's light wounds.",ch,NULL,victim,TO_CHAR);
	else
	{
            send_to_char("After your healing, ",ch);
	    if(!is_affected(victim,gsn_berserk))
	        show_char_status(victim,ch);
	    else
	        act("some of $N's wounds close.",ch,NULL,victim,TO_CHAR);
        }	    
       
    return TRUE;
}

bool spell_cure_poison( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;

    if ( !is_affected( victim, gsn_poison ) )
    {
        if (victim == ch)
          send_to_char("You aren't poisoned.\n\r",ch);
        else
          act("$N doesn't appear to be poisoned.",ch,NULL,victim,TO_CHAR);
        return FALSE;
    }

    if (check_dispel(level,victim,gsn_poison))
    {
        send_to_char("A warm feeling runs through your body.\n\r",victim);
        act("$n looks much better.",victim,NULL,NULL,TO_ROOM);
    }
    else
        send_to_char("Spell failed.\n\r",ch);

    return TRUE;
}

bool spell_cure_serious( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    int heal;

    if (checkLogorPreventHealing(ch, victim))
        return true;

    heal = 18 + (level / 4) + number_range(0, 4);
    checkMartyrsFire(ch, victim, heal);
    checkPhysickersInstinct(ch, heal);
    checkRevenant(*victim, heal);
    victim->hit = UMIN( victim->hit + heal, victim->max_hit );
    update_pos( victim );
    send_to_char( "You feel better!\n\r", victim );
    
    if ( ch != victim )
        if(IS_NPC(victim))
	    act("You cure $N's serious wounds.",ch,NULL,victim,TO_CHAR);
	else
	{
            send_to_char("After your healing, ",ch);
	    if(!is_affected(victim,gsn_berserk))
	        show_char_status(victim,ch);
	    else
	        act("some of $N's wounds close.",ch,NULL,victim,TO_CHAR);
        }	    
    return TRUE;
}

bool spell_currents( int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *)vo;
    EXIT_DATA *pexit;
    bool moved=FALSE;
    int door;
    int x=0, count=0;
    ROOM_INDEX_DATA *in_room, *to_room;

    if (ch->in_room == NULL)
        return FALSE;

    if (ch->in_room->sector_type != SECT_WATER_SWIM && ch->in_room->sector_type != SECT_WATER_NOSWIM
     && ch->in_room->sector_type != SECT_UNDERWATER)
    {
        send_to_char("There is no water to sweep into a frenzy here.\n\r", ch);
        return FALSE;
    }

    if (IS_NPC(victim) && (IS_SET(victim->act, ACT_NOSUBDUE) || IS_SET(victim->act, ACT_SENTINEL) || IS_SET(victim->nact, ACT_SHADE)))
    {
        act("$n resists the currents!", victim, NULL, NULL, TO_ROOM);
        return TRUE;
    }

    act ("$n summons a huge torrent of water to sweep you away!", ch, NULL, victim, TO_VICT);
    act ("You summon a huge torrent of water to sweep $N away!", ch, NULL, victim, TO_CHAR);
    act ("$n summons a huge torrent of water to sweep $N away!", ch, NULL, victim, TO_NOTVICT);

    if (number_percent() <= get_skill(victim, gsn_waveborne) || is_affected(victim, gsn_aquamove) || saves_spell(level, ch, victim, DAM_BASH))
    {
        send_to_char("The powerful currents tear at you, but you stand firm!\n", victim);
        act("$n stands firm within the currents!", victim, NULL, NULL, TO_ROOM);
        return true;
    }

    count = number_range(4, 14);

    while (x < count)
    {
        x++;
        in_room = victim->in_room;
        while (((pexit = in_room->exit[door = number_range(0, 5)]) == NULL) && (number_bits(5) != 0))
        {
            if (pexit == NULL)
                continue;
        }

        if (pexit == NULL)
            continue;

        if (!is_water_room(*pexit->u1.to_room))
            continue;

        if ( ( pexit   = in_room->exit[door] ) == NULL
        ||   ( to_room = pexit->u1.to_room   ) == NULL
        ||   !can_see_room(victim,pexit->u1.to_room))
            continue;

        if (IS_SET(pexit->exit_info, EX_CLOSED)
        &&  (!IS_AFFECTED(victim, AFF_PASS_DOOR) || IS_SET(pexit->exit_info,EX_NOPASS))
        &&   !IS_TRUSTED(victim,ANGEL) && !IS_PAFFECTED(victim, AFF_VOIDWALK))
            continue;

        act("$n is swept away on the rushing currents!", victim, NULL, NULL, TO_ROOM);
        char_from_room(victim);
        char_to_room(victim, to_room);
        moved = TRUE;
        act("$n is swept into the room on raging currents!", victim, NULL, NULL, TO_ROOM);
        do_look(victim, "auto");

        // Chance of dislodging inventory items, coins, and held items
        if (number_percent() > 15)
            continue;
        
        switch (number_range(0, 2))
        {
            case 0: // Held item
            {
                static const int checkSlots[] = {WEAR_WIELD, WEAR_DUAL_WIELD, WEAR_SHIELD, WEAR_HOLD};
                OBJ_DATA * obj(NULL);
                for (unsigned int i(0); i < (sizeof(checkSlots) / sizeof(checkSlots[0])); ++i)
                {
                    obj = get_eq_char(victim, checkSlots[i]);
                    if (obj == NULL) continue;
                    if (IS_OBJ_STAT(obj, ITEM_NOREMOVE) || IS_OBJ_STAT(obj, ITEM_NODROP) || IS_OBJ_STAT(obj, ITEM_NODISARM))
                    {
                        obj = NULL;
                        continue;
                    }

                    break;
                }

                // The grip skill and raw strength can keep the item from being dislodged
                if (obj != NULL && number_percent() > get_skill(victim, gsn_grip) && number_percent() > (get_curr_stat(victim, STAT_STR) * 2))
                {
                    act("The rushing currents tear $p from your grasp!", victim, obj, NULL, TO_CHAR);
                    act("The rushing currents tear $p from $n's grasp!", victim, obj, NULL, TO_ROOM);
                    obj_from_char(obj);
                    obj_to_room(obj, victim->in_room);
                }
                break;
            }

            case 1: // Inventory item
            {
                // Build a list of candidate items
                std::vector<OBJ_DATA*> objs;
                for (OBJ_DATA * obj(victim->carrying); obj != NULL; obj = obj->next_content)
                {
                    if (!IS_OBJ_STAT(obj, ITEM_NODROP) && !IS_OBJ_STAT(obj, ITEM_WIZI) && !obj->worn_on)
                        objs.push_back(obj);
                }

                // Choose a random obj
                if (!objs.empty())
                {
                    OBJ_DATA * obj(objs[number_range(0, objs.size() - 1)]);
                    act("The raging waters tear $p from you!", victim, obj, NULL, TO_CHAR);
                    act("The raging waters tear $p from $n!", victim, obj, NULL, TO_ROOM);
                    obj_from_char(obj);
                    obj_to_room(obj, victim->in_room);
                }

                break;
            }

            case 2: // Coins
            {
                // Determine how many coins are dropped
                long coins[MAX_COIN];
                long totalCoins(0);
                for (unsigned int i(0); i < MAX_COIN; ++i)
                {
                    // Random number of coins up to the max; however, platinum has some restrictions
                    coins[i] = number_range(0, victim->coins[i]);
                    if (i == C_PLATINUM) 
                        coins[i] = UMIN(coins[i], IS_NPC(victim) ? 2 : 6);

                    if (coins[i] > 0)
                    {
                        totalCoins += coins[i];
                        obj_to_room(create_money(coins[i], i), victim->in_room);
                    }
                }
                
                if (totalCoins > 0)
                {
                    std::ostringstream mess;
                    mess << "The roiling currents dislodge " << coins_to_str(coins) << " from your purse!";
                    act(mess.str().c_str(), victim, NULL, NULL, TO_CHAR);

                    if (totalCoins == 1) act("A single coin is thrown from $n's purse by the roiling currents.", victim, NULL, NULL, TO_ROOM);
                    else if (totalCoins == 2) act("A couple of coins are swept from $n's purse by the roiling currents.", victim, NULL, NULL, TO_ROOM);
                    else act("Several coins are torn from $n's purse by the roiling currents!", victim, NULL, NULL, TO_ROOM);
                    
                    dec_player_coins(victim, coins);
                }
                break;
            }
        }
    }

    if (moved)
    {
        WAIT_STATE(victim, UMAX(victim->wait, PULSE_VIOLENCE));
        act("$n coughs as $e catches $s breath after being washed around by the current.", victim, NULL, NULL, TO_ROOM);
        act("You cough as you catch your breath after being washed around by the current.", victim, NULL, NULL, TO_CHAR);
    }
    else
        send_to_char("The currents are unable to wash you away.\n\r", victim);

    return TRUE;
}

bool check_encasecasting(int level, CHAR_DATA * ch, CHAR_DATA * victim)
{
    // Check for already encased
    if (IS_OAFFECTED(victim, AFF_ENCASE))
    {
        if (ch != NULL)
            act("$N is already encased in ice.", ch, NULL, victim, TO_CHAR);

        return false;
    }

    // Do damage
    if (ch == NULL) sourcelessDamage(victim, "the wave of frost", number_range(1, level), gsn_encase, DAM_COLD);
    else damage(ch, victim, number_range(1, level), gsn_encase, DAM_COLD, true);
    
    // Check saves
    if (saves_spell(level, ch, victim, DAM_COLD) || (IS_NPC(victim) && IS_SET(victim->act, ACT_NOSUBDUE)))
    {
        send_to_char("The wave of frost begins to encase you in ice, but you break free!\n", victim);
        act("The wave of frost begins to encase $n in ice, but $e breaks free!", victim, NULL, NULL, TO_ROOM);
        return true;
    }

    if (check_spirit_of_freedom(victim))
    {
	    send_to_char("The spirit of freedom surges within you, and the encasing ice melts away.\n", victim);
    	act("The encasing ice falls away from $n's body.", victim, NULL, NULL, TO_ROOM);
	    affect_strip(victim, gsn_encase);
    	return true;
    }
    
    // Prepare the effect
    AFFECT_DATA af = {0};
    af.type     = gsn_encase;
    af.level    = level;

    // Check for the effect already being present
    AFFECT_DATA * paf(get_affect(victim, gsn_encase));
    if (paf == NULL)
    {
        af.where    = TO_AFFECTS;
        af.duration = (level / 10);
        af.location = APPLY_DEX;
        af.modifier = -3;
        af.bitvector = AFF_SLOW;
        affect_to_char(victim, &af);

        send_to_char("Ice hardens around your body, hampering your movement.\n", victim);
        act("Ice forms, hardening around $n's body.", victim, NULL, NULL, TO_ROOM);
        return true;
    }
    
    // Effect already present, time to fully-encase
    affect_strip(victim, gsn_encase);
    
    af.where        = TO_OAFFECTS;
    af.bitvector    = AFF_ENCASE;
    af.duration     = (level / 17);
	affect_to_char(victim, &af);

	af.where        = TO_PAFFECTS;
	af.bitvector    = AFF_AIRLESS;
	affect_to_char(victim, &af);
    
    SET_BIT(victim->act, PLR_FREEZE);
    stop_fighting_all(victim);
    
    act("$n's body becomes completely encased in ice, immobilizing $m!", victim, NULL, NULL, TO_ROOM);
    send_to_char("Ice completely encases your body, immobilizing you!\n", victim);

    return true;

}

bool spell_encase( int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    act("You send a wave of frost at $N!", ch, NULL, victim, TO_CHAR);
    act("$n sends a wave of frost at $N!", ch, NULL, victim, TO_NOTVICT);
    act("$n sends a wave of frost at you!", ch, NULL, victim, TO_VICT);

    return check_encasecasting(level, ch, victim);
}

static void fillRoomWithWater(int sn, int level, int duration, bool underwater, ROOM_INDEX_DATA * room)
{
    // Wipe out blazing inferno
    if (room_is_affected(room, gsn_blazinginferno))
    {
        if (room->people != NULL) act("The rush of water extinguishes the flames of the inferno!", room->people, NULL, NULL, TO_ALL);
        room_affect_strip(room, gsn_blazinginferno);
    }

    // Wipe out blaze
    if (room_is_affected(room, gsn_blaze))
    {
        if (room->people != NULL) act("The surging waters snuff out the blaze here!", room->people, NULL, NULL, TO_ALL);
        room_affect_strip(room, gsn_blaze);
    }
   
    // Check for pyre destruction
    OBJ_DATA * pyre(lookupPyre(room));
    if (pyre != NULL)
    {
        if (room->people != NULL) act("The flood of water extinguishes $p!", room->people, pyre, NULL, TO_ALL);
        extract_obj(pyre);
    }

    // Check for plant growth 
    if (room_is_affected(room, gsn_plantgrowth))
    {
    	if (room->people != NULL) act("The flooding waters rip through this place, dispersing the plant growth.", room->people, NULL, NULL, TO_ALL);
    	room_affect_strip(room, gsn_plantgrowth);
    }

    // Unhide folks
    CHAR_DATA * vch_next;
    for (CHAR_DATA * vch(room->people); vch != NULL; vch = vch_next)
    {
    	vch_next = vch->next_in_room;

	    if (unhide_char(vch) || uncamo_char(vch))
    	{
	        act("You are revealed by the rushing water!", vch, NULL, NULL, TO_CHAR);
	        act("$n is revealed by the rushing water!", vch, NULL, NULL, TO_ROOM);
    	}
    }

    // Apply the effect to the room and change the sector
    AFFECT_DATA af = {0};
    af.where    = TO_ROOM;
    af.type	    = sn;
    af.level	= level;
    af.duration = duration;
    af.modifier = room->sector_type;
    affect_to_room(room, &af);

    room->sector_type = (underwater ? SECT_UNDERWATER : SECT_WATER_SWIM);
}

bool spell_flood( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
    // Check for cooldown
    for (AFFECT_DATA * paf(get_affect(ch, sn)); paf != NULL; paf = get_affect(ch, sn, paf))
	{
        if (paf->modifier == 0)
    	{
	        send_to_char("You are not yet ready to summon another flood.\n", ch);
	        return false;
    	}
    }

    // Check for terrain type
    if (ch->in_room == NULL || ch->in_room->sector_type == SECT_UNDERWATER || ch->in_room->sector_type == SECT_AIR)
    {
        send_to_char("You cannot summon a flood here.\n", ch);
        return false;
    }

    // Echo the flood
    act("$n raises $s arms with a shout, and a flood of water rushes up!", ch, NULL, NULL, TO_ROOM);
    send_to_char("You raise your arms with a shout, summoning a flood of water!\n", ch);

    // Check for slowed movement
    CHAR_DATA * victim_next;
    for (CHAR_DATA * victim(ch->in_room->people); victim != NULL; victim = victim_next)
    {
        // Check for safe vs the spell
        victim_next = victim->next_in_room;
    	if (is_safe_spell(ch, victim, true))
	        continue;

        // Stop the fighting, then check disqualifiers for the slow effect
    	send_to_char( "You are caught in the rush of water.\n", victim);
	    act("$n is caught in the rush of water.", victim, NULL, NULL, TO_ROOM);
    	stop_fighting_all(victim);
        if (is_same_group(ch, victim) || is_affected(victim, sn) || IS_AFFECTED(victim, AFF_SLOW) || number_percent() <= get_skill(victim, gsn_waveborne) || is_affected(victim, gsn_aquamove))
            continue;
       
        // Check for a save 
        if (saves_spell(level, ch, victim, DAM_OTHER))
        {
            send_to_char("You flounder momentarily in the water, but quickly recover.\n", victim);
            act("$n flounders momentarily in the water, but quickly recovers.", victim, NULL, NULL, TO_ROOM);
            continue;
        }

        // Failed to save, so strip haste and/or apply slow
        if (IS_AFFECTED(victim, AFF_HASTE))
        {
            AFFECT_DATA * paf_next;
    	    for (AFFECT_DATA * paf(victim->affected); paf != NULL; paf = paf_next)
	        {
		        paf_next = paf->next;
        		if (paf->where == TO_AFFECTS && paf->bitvector == AFF_HASTE)
		            affect_remove(ch, paf);
	        }
	    }
    	else
	    {
            AFFECT_DATA af = {0};
            af.where     = TO_AFFECTS;
            af.type      = gsn_flood;
	        af.location	 = APPLY_DEX;
    	    af.modifier	 = -3;
	        af.bitvector = AFF_SLOW;
            af.duration  = level / 17;
	        affect_to_char(victim, &af);
    	}

        send_to_char("You flounder in the water, slowing down as you quickly tire.\n", victim);
    	act("$n flounders in the water, slowing down as $e quickly tires.", victim, NULL, NULL, TO_ROOM);
    }

    // Initialize some values
    int duration(2);
    int cooldown(25);
    int roomCount((level / 17) + number_range(0, 3));

    // Check for deluge
    if (number_percent() <= get_skill(ch, gsn_deluge))
    {
        roomCount += number_range(3, 8);
        duration = 4;
        cooldown = 20;
        check_improve(ch, NULL, gsn_deluge, true, 1);
    }
    else
        check_improve(ch, NULL, gsn_deluge, false, 1);

    // Flood this room
    std::vector<ROOM_INDEX_DATA*> floodedRooms;
    floodedRooms.push_back(ch->in_room);
    fillRoomWithWater(sn, level, duration + number_range(0, 2), true, ch->in_room);

    // Flood the rest of the rooms
    while (roomCount > 0 && !floodedRooms.empty())
    {
        // Choose a random room from the flooded list
        size_t index(number_range(0, floodedRooms.size() - 1));
        ROOM_INDEX_DATA * room(floodedRooms[index]);

        // Build a list of possible directions to flood into
        std::vector<ROOM_INDEX_DATA*> nextRooms;
        for (unsigned int i(0); i < Direction::Max; ++i)
        {
            // Never flood upwards
            if (i == Direction::Up)
                continue;

            // Check the room in this direction, if any valid
            // May not be air or underwater, or already flooded
            ROOM_INDEX_DATA * nextRoom(Direction::Adjacent(*room, static_cast<Direction::Value>(i)));
            if (nextRoom != NULL && nextRoom->sector_type != SECT_AIR && nextRoom->sector_type != SECT_UNDERWATER && (roomCount > 4 || !is_water_room(*nextRoom)) && !room_is_affected(nextRoom, sn))
                nextRooms.push_back(nextRoom);
        }

        // If there are no candidate rooms, remove this room from the flooded list
        if (nextRooms.empty())
        {
            floodedRooms[index] = floodedRooms[floodedRooms.size() - 1];
            floodedRooms.pop_back();
            continue;
        }

        // Choose a room at random to flood into
        ROOM_INDEX_DATA * nextRoom(nextRooms[number_range(0, nextRooms.size() - 1)]);
        fillRoomWithWater(sn, level, duration + number_range(0, 2), roomCount > 4, nextRoom);
        floodedRooms.push_back(nextRoom);
        --roomCount;
    }

    // Apply cooldown
    AFFECT_DATA af = {0};
    af.where	= TO_AFFECTS;
    af.type	    = sn;
    af.level	= level;
    af.duration	= cooldown;
    affect_to_char(ch, &af);

    return true;
}

bool spell_freeze( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    CHAR_DATA *victim;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
    char buf[MAX_STRING_LENGTH];

    if (!ch->in_room)
	return FALSE;

    af.where	 = TO_AFFECTS;
    af.type	 = sn;
    af.level	 = level;
    af.duration	 = level/5;
    af.location  = APPLY_DEX;
    af.bitvector = 0;

    if (ch->in_room->sector_type == SECT_UNDERWATER)
    {
	act("The water around you begins to freeze!", ch, NULL, NULL, TO_ROOM);
	act("You attempt to freeze the water around you.", ch, NULL, NULL, TO_CHAR);
	for (victim = ch->in_room->people; victim; victim = victim->next_in_room)
	{
	    if (is_same_group(victim, ch) || is_safe_spell(ch, victim, TRUE) || (!IS_NPC(victim) && !IS_PK(ch, victim)))
		continue;

	    check_killer(ch, victim);
	    if ((ch->fighting != victim) && (victim->fighting != ch) 
	     && can_see(victim, ch) && !IS_NPC(victim))
		sprintf(buf, "Help!  %s is attacking me!", PERS(ch, victim));
	    else
		sprintf(buf, "Help!  Someone is attacking me!");
	    do_autoyell(victim, buf);
	    
	    if (!victim->fighting)
		victim->fighting = ch;
            
	    send_to_char("You feel slower as coldness spreads through you.\n\r", victim);
	    act("$n turns slightly blue.", victim, NULL, NULL, TO_ROOM);
	    if (is_affected(victim, gsn_freeze))
	    {
		af.modifier = get_modifier(victim->affected, gsn_freeze) - 2;
		affect_strip(victim, gsn_freeze);
	    }
	    else
		af.modifier = -2;
            affect_to_char(victim, &af);

	    if (saves_spell(level + 5, ch, victim, DAM_COLD))
		continue;

	    send_to_char("The ice surrounds you, hindering your movement!\n\r", victim);
	    act("Ice surrounds $n, hindering $s movement.", victim, NULL, NULL, TO_ROOM);
            WAIT_STATE(victim, UMAX(victim->wait, 2*PULSE_VIOLENCE));

	}
    }
    else if ((ch->in_room->sector_type == SECT_WATER_SWIM)
	  || (ch->in_room->sector_type == SECT_WATER_NOSWIM))
    {
	if (room_is_affected(ch->in_room, gsn_freeze))
	    send_to_char("The water here is already frozen.\n\r", ch);
	else
	{
	    af.where     = TO_ROOM;
	    af.modifier  = 40;
	
	    send_to_char("You summon a wave of frost to freeze the water below you.\n\r", ch);
	    act("The water below you freezes as a wave of frost passes over it.", ch, NULL, NULL, TO_ROOM);

	    affect_to_room(ch->in_room, &af);
	}
    }
    else if (ch->in_room->sector_type == SECT_AIR
      || ch->in_room->sector_type == SECT_DESERT)
    {
	send_to_char("There isn't enough water to freeze here.\n\r",ch);
	return FALSE;
    }
    else
    {
	if (room_is_affected(ch->in_room, gsn_freeze))
	    send_to_char("There is already a sheen of ice here.\n\r", ch);
	else
	{
	    af.where     = TO_ROOM;
	    af.modifier  = 20;
	    af.duration	 = af.duration/2;
	    
	    send_to_char("You summon a sheen of thin ice to coat the ground.\n\r", ch);
	    act("A sheen of thin ice develops on the ground.", ch, NULL, NULL, TO_ROOM);

	    affect_to_room(ch->in_room, &af);
	}
    }

    return TRUE;
}	
	    
bool spell_frostbite( int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *)vo;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if (is_affected(victim, gsn_frostbite))
    {
	if (victim == ch)
	    send_to_char("You are already freezing.\n\r", ch);
	else
            send_to_char("They are already freezing.\n\r", ch);
        return FALSE;
    }

    if ( saves_spell( level+1, ch, victim, DAM_OTHER ) )
    {
        act ("Your hands chill for a moment, but you resist the spell.", victim, NULL, NULL, TO_CHAR);
        act ("$N resists your spell.", ch, NULL, victim, TO_CHAR);
        return TRUE;
    }

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = level/11;
    af.location  = 0;
    af.modifier  = 0;
    af.bitvector = 0;
    affect_to_char(victim, &af);

    act("$n shivers as $s lips turn blue and fingers tighten up with cold!", victim, NULL, NULL, TO_ROOM);
    act("You shiver as cold penetrates you!", victim, NULL, NULL, TO_CHAR);

    return TRUE;
}

bool spell_frostbrand( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    OBJ_DATA *vObj = (OBJ_DATA *) vo;
    AFFECT_DATA af;

    if (!(vObj->item_type == ITEM_WEAPON || vObj->item_type == ITEM_ARROW ))
    {
	send_to_char("Only weapons may be given a frostbrand.\n\r", ch);
	return FALSE;
    }

    if (obj_is_affected(vObj, gsn_frostbrand)
     || (vObj->value[3] == attack_lookup("frbite"))) 
    {
	send_to_char("That weapon is already surrounded in a cold aura.\n\r", ch);
	return FALSE;
    }

    if (IS_SET(vObj->extra_flags[0], ITEM_EVIL)
     || IS_SET(vObj->extra_flags[0], ITEM_ANTI_GOOD)
     || IS_SET(vObj->extra_flags[0], ITEM_DARK))
    {
	act("The dark aura surrounding $p prevent your attempts to frostbrand it.", ch, vObj, NULL, TO_CHAR);
	return FALSE;
    }

    if (vObj->value[3] == DAM_FIRE)
    {
	send_to_char("You cannot frostbrand a flaming weapon.\n\r", ch);
	return FALSE;
    }

    if (vObj->value[3] == DAM_COLD)
    {
	send_to_char("That weapon is already cold.\n\r", ch);
	return FALSE;
    }

    af.where	 = TO_OBJECT;
    af.type	 = sn;
    af.level	 = level;
    af.modifier  = 0 /*vObj->value[3]*/;
    af.location  = 0;
    af.duration  = level/3;
    af.bitvector = 0;
    af.point	 = NULL /*(void *) vObj->obj_str*/;
    affect_to_obj(vObj, &af);

    act("$p begins emitting a cold aura.", ch, vObj, NULL, TO_CHAR);
    act("$n chants, and $p begins emitting a cold aura.", ch, vObj, NULL, TO_ROOM);

    return TRUE;
}

bool spell_heal( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;

    if (checkLogorPreventHealing(ch, victim))
        return true;

    int heal = 100;
    checkMartyrsFire(ch, victim, heal);
    checkPhysickersInstinct(ch, heal);
    checkRevenant(*victim, heal);
    victim->hit = UMIN( victim->hit + heal, victim->max_hit );
    update_pos( victim );
    send_to_char( "A warm feeling fills your body.\n\r", victim );
   
    if ( ch != victim )
        if(IS_NPC(victim))
	    act("You heal $N.",ch,NULL,victim,TO_CHAR);
	else
	{
            send_to_char("After your healing, ",ch);
	    if(!is_affected(victim,gsn_berserk))
	        show_char_status(victim,ch);
	    else
	        act("some of $N's wounds close.",ch,NULL,victim,TO_CHAR);
        }	    
    return TRUE;
}

bool spell_heatsink( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    OBJ_DATA *weapon;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if ((weapon = get_eq_char(victim, WEAR_WIELD)) == NULL)
        if ((weapon = get_eq_char(victim, WEAR_DUAL_WIELD)) == NULL)
        {
            act("$N isn't using a weapon, so you have nothing to ice over!", ch, NULL, victim, TO_CHAR);
            return FALSE;
        }

    if (IS_SET(weapon->extra_flags[0], ITEM_VIS_DEATH))
    {
        act("$N isn't using a weapon, so you have nothing to ice over!", ch, NULL, victim, TO_CHAR);
	return FALSE;
    }

    if (saves_spell(level+4, ch, victim, DAM_OTHER))
    {
        act("$N resists your spell.", ch, NULL, victim, TO_CHAR);
        return TRUE;
    }

    af.where     = TO_OBJECT;
    af.type      = sn;
    af.level     = level;
    af.duration  = level/12;
    af.location  = 0;
    af.modifier  = 0;
    af.bitvector = 0;
    affect_to_obj( weapon, &af );
    act("$n's weapon is coated in frost as it is chilled to the core.", victim, NULL, NULL, TO_ROOM);
    act("Your weapon is coated in frost as it is chilled to the core.", victim, NULL, NULL, TO_CHAR);
    return TRUE;
}

bool spell_holywater( int sn, int level, CHAR_DATA *ch, void *vo,int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;

    int dam = dice(level, 9);
    act("$n hurls holy water at $N!", ch, NULL, victim, TO_NOTVICT);
    act("$n hurls holy water at you!", ch, NULL, victim, TO_VICT);
    act("You hurl holy water at $N!", ch, NULL, victim, TO_CHAR);

    bool revenant(number_percent() <= get_skill(victim, gsn_revenant));
    if (!IS_NPC(victim))
    {
        if (revenant)
            dam = (dam * 4) / 10;
        else
        {
            act( "$N does not seem to be affected.", ch, NULL, victim, TO_CHAR );
            return FALSE;
        }
    }
    else if (!IS_SET(victim->act, ACT_UNDEAD) && !is_demon(victim))
    {
        if (revenant)
            dam /= 4;
        else
        {
            act( "$N does not seem to be affected.", ch, NULL, victim, TO_CHAR );
    	    damage(ch, victim, 0, sn, DAM_HOLY, FALSE);
            return TRUE;
        }
    }

    if ( saves_spell( level, ch, victim,DAM_HOLY) )
        dam /= 2;
    else if (number_percent() < 5 && IS_NPC(victim) && !IS_SET(victim->act, ACT_NOSUBDUE))
    {
        act("$N cries in terror as the holy water burns right through $M!", ch, NULL, victim, TO_NOTVICT);
        act("$N cries in terror as the holy water burns right through $M!", ch, NULL, victim, TO_CHAR);
        act("You cry in terror as the holy water burns right through you!", ch, NULL, victim, TO_VICT);
        kill_char(victim, ch);
        return TRUE;
    }

    damage_old( ch, victim, dam, sn, DAM_HOLY ,TRUE);
    return TRUE;
}

bool spell_iceblast( int sn, int level, CHAR_DATA *ch, void *vo,
        int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    int dam;

    dam = dice( level, 6 );
    if ( saves_spell( level, ch, victim, DAM_COLD ) )
        dam /= 2;
    act ("$n unleashes a blast of ice upon $N!",ch,NULL,victim,TO_NOTVICT);
    act ("You unleash a blast of ice upon $N!",ch,NULL,victim,TO_CHAR);
    act ("$n unleashes a blast of ice upon you!",ch,NULL,victim,TO_VICT);
    damage_old( ch, victim, dam, sn,DAM_COLD,TRUE);
    return TRUE;
}

bool spell_icebolt( int sn, int level, CHAR_DATA *ch, void *vo, int target)
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
	send_to_char("Who are you trying to ice bolt?\n\r", ch);
	return FALSE;
    }

    if (direction[0] == '\0')
        pRoom = ch->in_room;
    else
    {
	if (ch->fighting)
	{
	    send_to_char("You can't shoot an ice bolt into another room while fighting!\n\r", ch);
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
	 || IS_SET(ch->in_room->exit[dir]->exit_info, EX_ICEWALL) 
	 || IS_SET(ch->in_room->exit[dir]->exit_info, EX_WALLOFFIRE) 
	 || IS_SET(ch->in_room->exit[dir]->exit_info, EX_WALLOFVINES)
	 || IS_SET(ch->in_room->exit[dir]->exit_info, EX_FAKE))
        {
            send_to_char("You can't shoot an icebolt in that direction!\n\r",ch);
            return FALSE;
        }

	pRoom = ch->in_room->exit[dir]->u1.to_room;
    }

    if (pRoom == NULL)
    {
        send_to_char("You cannot shoot your icebolt in that direction!\n\r",ch);
        return FALSE;
    }

    if (room_is_affected(ch->in_room, gsn_smoke)  
     || room_is_affected(pRoom, gsn_smoke))
    {
        send_to_char("The thick smoke wafting by blocks you from targetting your icebolt.\n\r",ch);
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
		send_to_char("Shoot an ice bolt at whom?\n\r", ch);
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

    dam = dice(level, 4) + dice((2 * level) / 3, 1);

    if (saves_spell(level, ch, victim, DAM_COLD))
	dam /= 2;

    if (pRoom != ch->in_room)
	dam /= 2;

    /* 1/2 damage if they save, 1/2 damage again if it's one room away. */

    act("You throw your hands wide and channel the powers of ice into a single bolt!",ch,NULL,NULL,TO_CHAR);

    if (pRoom == ch->in_room)
    {
        act("$n throws $s hands wide, channeling $s powers into an icebolt, which streaks towards $N.",ch,NULL,victim,TO_NOTVICT);
        act("$n throws $s hands wide, and a bolt of pure ice streaks through the air towards you!",ch,NULL,victim,TO_VICT);
    }
    else
    {
	char buf[MAX_STRING_LENGTH];

	sprintf(buf, "$n throws $s hands wide, channeling $s powers into an icebolt, which streaks %s!", (dir == 0 ? "northwards" 
			: dir == 1 ? "eastwards"
			: dir == 2 ? "southwards"
			: dir == 3 ? "westwards"
			: dir == 4 ? "upwards"
			: dir == 5 ? "down below you" : "away"));
	act(buf, ch, NULL, NULL, TO_ROOM); 
        act("A bolt of ice appears and streaks towards $n suddenly!", victim, NULL, NULL, TO_ROOM);
	act("A bolt of ice appears and streaks towards you!", victim, NULL, NULL, TO_CHAR);
    }

    if ((pRoom != ch->in_room) && IS_NPC(victim) && ((IS_SET(victim->act, ACT_SENTINEL) || IS_SET(victim->act, ACT_NOTRACK))) && (victim->hit < victim->max_hit))
	dam = 0;
    if (pRoom != ch->in_room
      && check_defensiveroll(victim))
    {
	send_to_char("You roll as the bolt of ice strikes you, lessening its affect.\n\r",victim);
	dam /= 2;
    }
    damage_old( ch, victim, dam, sn,DAM_COLD,TRUE);

    return TRUE;
}
    
bool spell_iceshard( int sn, int level, CHAR_DATA *ch,void *vo,int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;

    act("$n conjures a small spray of icicles to spear $N.", ch, NULL, victim, TO_ROOM);

    int repeatChance(0);
    AFFECT_DATA * paf(get_affect(ch, gsn_arcticchill));
    if (paf != NULL && paf->modifier == 0)
        repeatChance = 60;

    AFFECT_DATA af = {0};
    af.where    = TO_AFFECTS;
    af.type     = gsn_rimeshard;
    af.level    = level;
    af.location = APPLY_RESIST_COLD;
    af.duration = (level / 8);

    while (true)
    {
        // Fire an ice shard
        int dam(25 + (level / 2));
        dam = (number_range(dam / 2, dam * 2));
        if (saves_spell(level, ch, victim, DAM_COLD))
            dam /= 2;

        damage_old(ch, victim, dam, sn, DAM_COLD, true);

        // Check for death (by presence in room)
        if (ch->in_room != victim->in_room)
            return true;

        // Check for rimeshard
        if (number_percent() <= (get_skill(ch, gsn_rimeshard) / 2))
        {
            // Passed the skill check, apply or intensify the effect
            AFFECT_DATA * paf(get_affect(victim, gsn_rimeshard));
            if (paf == NULL)
            {
                // Effect is absent, so apply it
                send_to_char("You shiver as your body is briefly dusted with frost.\n", victim);
                act("$n shivers as $s body is briefly dusted with frost.", victim, NULL, NULL, TO_ROOM);

                af.modifier = -1;
                affect_to_char(victim, &af);
                check_improve(ch, victim, gsn_rimeshard, true, 4);
            }
            else if (paf->modifier > -20)
            {
                // Effect is present but not maxed, so intensify (and renew) it
                send_to_char("Your skin rimes over, intensifying the chill!\n", victim);
                if (ch != victim) act("$N's skin rimes over, intensifying the chill!", ch, NULL, victim, TO_CHAR);

                af.duration = UMAX(af.duration, paf->duration);
                af.modifier = UMAX(-20, paf->modifier - 1);
                affect_strip(victim, gsn_rimeshard);
                affect_to_char(victim, &af);
                check_improve(ch, victim, gsn_rimeshard, true, 4);
            }
            else // Effect is present but maxed, so just renew the duration
                paf->duration = UMAX(af.duration, paf->duration);
        }
        else
            check_improve(ch, victim, gsn_rimeshard, false, 4);

        // Check for frostbite
        if (!is_affected(victim, gsn_frostbite))
        {
            if (saves_spell(level, ch, victim, DAM_COLD))
            {
                send_to_char("Your hands chill for a moment, but warm again swiftly.\n", victim);
                if (ch != victim) act("$N's joints tremble from cold, but only briefly.", ch, NULL, victim, TO_CHAR);
            }
            else
            {
                AFFECT_DATA af = {0};
                af.where    = TO_AFFECTS;
                af.type     = gsn_frostbite;
                af.level    = level;
                af.duration = (level / 11);
                affect_to_char(victim, &af);

                act("$n shivers as $s lips turn blue and joints tighten up with cold!", victim, NULL, NULL, TO_ROOM);
                act("You shiver as cold penetrates you!", victim, NULL, NULL, TO_CHAR);
            }
        }

        // Check for a repeat
        if (number_percent() > repeatChance)
            break;

        repeatChance /= 2;
    }

    return true;
}

bool spell_icestorm( int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
    DESCRIPTOR_DATA *d;

    if (is_affected(ch, gsn_icestorm))
    {
        send_to_char("You cannot summon another ice storm yet.\n\r", ch);
        return FALSE;
    }

    if (!ch->in_room)
        return FALSE;

    if (area_is_affected(ch->in_room->area, gsn_icestorm))
    {
        send_to_char("An ice storm already rages through here.\n\r", ch);
        return FALSE;
    }

    if (ch->in_room->sector_type == SECT_UNDERWATER)
    {
        send_to_char("You can't call an ice storm underwater.\n\r", ch);
        return FALSE;
    }

    af.where        = TO_AREA;
    af.type         = sn;
    af.level        = level;
    af.duration     = 8;
    af.location     = 0;
    af.modifier     = 0;
    af.bitvector   = 0;
    affect_to_area(ch->in_room->area, &af);

    af.duration     = 90;
    affect_to_char(ch, &af);

    for (d = descriptor_list; d != NULL; d = d->next)
    {
        if (d->connected == CON_PLAYING && d->character->in_room && ch->in_room
         && (d->character->in_room->area == ch->in_room->area))
            send_to_char("An ominous wind arises, as the chill air signals the coming of an ice storm!\n\r", d->character);
    }

    // Force a weather update
    weather_update();
    return TRUE;
}

// need to add a mount check here

bool spell_icyprison( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
    ROOM_INDEX_DATA *prison;
    OBJ_DATA *pObj;

    if (!ch->in_room)
	return FALSE;

    if (room_is_affected(ch->in_room, gsn_icyprison))
    {
	send_to_char("You cannot form an icy prison from within an icy prison.\n\r", ch);
	return FALSE;
    }

    if (((IS_NPC(victim) && IS_SET(victim->act, ACT_NOSUBDUE)) || (number_percent() < (get_curr_stat(victim, STAT_DEX) * 2.5))) && (victim != ch))
    {
	act("Walls of ice begin to spring up around $n, but $e deftly escapes!", victim, NULL, NULL, TO_ROOM);
	send_to_char("Walls of ice begin to form around you, but you deftly escape!\n\r", victim);
	return TRUE;
    }

    act("Walls of ice spring up around $n, imprisoning $m within!", victim, NULL, NULL, TO_ROOM);
    send_to_char("Walls of ice spring up around you, imprisoning you within!\n\r", victim);

    if (check_spirit_of_freedom(victim))
    {
	send_to_char("The spirit of freedom surges within you, and the icy prison melts away!\n\r", victim);
	act("The icy prison surrounding $n quickly melts away.", victim, NULL, NULL, TO_ROOM);
	return TRUE;
    }

    prison = new_room_area(victim->in_room->area);
    
    prison->name 	= str_dup("An Icy Prison");
    prison->description = str_dup("Thick walls of ice surround you on every side, their encompassing mass\n\rpreventing your escape.  The interior of this place is extremely cold and\n\renclosed, sapping away at your strength.\n\r");
    prison->room_flags  = ROOM_NOGATE|ROOM_NOSUM_TO|ROOM_NOSUM_FROM|ROOM_NOWEATHER;
    prison->sector_type	= ch->in_room->sector_type;

    if (IS_SET(ch->in_room->room_flags, ROOM_NO_RECALL))
	SET_BIT(prison->room_flags, ROOM_NO_RECALL);

    af.where	 = TO_ROOM;
    af.type	 = sn;
    af.level	 = level;
    af.modifier	 = (victim->was_in_room ? victim->was_in_room->vnum : victim->in_room->vnum);
    af.location  = APPLY_NONE;
    af.duration  = level/10;
    af.bitvector = 0;

    if ((pObj = create_object(get_obj_index(OBJ_VNUM_ICY_PRISON), level)) == NULL)
	bug("icy prison: could not load prison object", 0);

    af.point = (void *) pObj;
    affect_to_room(prison, &af);

    af.duration = -1;
    af.point = (void *) prison;
    affect_to_obj(pObj, &af);
    obj_to_room(pObj, victim->in_room);

    if (victim->in_room->vnum != 0)
        victim->was_in_room = victim->in_room;

    char_from_room(victim);
    char_to_room(victim, prison);

    return TRUE;
}
    

bool spell_icyshield( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    OBJ_DATA *shield;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if (is_affected(ch, sn))
    {
	send_to_char("You don't feel ready to create another icy shield yet.\n\r", ch);
	return FALSE;
    }

    if (!(shield = create_object(get_obj_index(OBJ_VNUM_ICY_SHIELD), level)))
    {
	bug("Spell: icy shield.  Cannot load shield object.", 0);
	send_to_char("Something seems to be amiss...\n\r", ch);
	return FALSE;
    }

    shield->level 	= level;
    shield->value[0]	= level/6;
    shield->value[1]	= level/5;
    shield->value[2]    = level/4;
    shield->value[3]    = level/5 + 1;

    af.where	 = TO_AFFECTS;
    af.type	 = sn;
    af.level	 = level;
    af.duration  = 25;
    af.location  = 0;
    af.modifier  = 0;
    af.bitvector = 0;
    affect_to_char(ch, &af);

    af.where	 = TO_OBJECT;
    af.duration  = -1;
    af.location	 = APPLY_HITROLL;
    af.modifier  = level/10;
    affect_to_obj(shield, &af);

    af.location  = APPLY_DAMROLL;
    affect_to_obj(shield, &af);

    af.location  = APPLY_RESIST_FIRE;
    af.modifier  = (8 + (level / 3));
    affect_to_obj(shield, &af);

    act("Raising your arms above you, $p shimmers into your waiting hands.", ch, shield, NULL, TO_CHAR);
    act("$n raises $s arms above $m, and $p shimmers into $s waiting hands.", ch, shield, NULL, TO_ROOM);

    obj_to_char(shield, ch);

    return TRUE;
} 

bool spell_lifeshield( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
    OBJ_DATA *shield;

    if (is_affected(ch, sn))
    {
	send_to_char("You don't feel ready to create another life shield yet.\n\r", ch);
	return FALSE;
    }

    if (!(shield = create_object(get_obj_index(OBJ_VNUM_LIFE_SHIELD), level)))
    {
	bug("Spell: life shield.  Cannot load shield object.", 0);
	send_to_char("Something seems to be amiss...\n\r", ch);
	return FALSE;
    }

    shield->level 	= level;
    shield->timer	= level * 20;
    shield->value[0]	= level/6 - 1;
    shield->value[1]	= level/4;
    shield->value[2]    = level/5 + 1;
    shield->value[3]    = level/5 - 1;

    af.where	 = TO_OBJECT;
    af.type	 = sn;
    af.level	 = level;
    af.duration  = 25;
    af.location  = 0;
    af.modifier  = 0;
    af.bitvector = 0;
    affect_to_char(ch, &af);

    af.duration  = -1;
    af.location	 = APPLY_HIT;
    af.modifier  = level/2;
    affect_to_obj(shield, &af);

    act("Raising your arms above you, $p shimmers into your waiting hands.", ch, shield, NULL, TO_CHAR);
    act("$n raises $s arms above $m, and $p shimmers into $s waiting hands.", ch, shield, NULL, TO_ROOM);

    obj_to_char(shield, ch);

    return TRUE;
} 

bool spell_massheal( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
    CHAR_DATA *victim;

    ch->hit = UMIN( ch->hit + 100, ch->max_hit );
    send_to_char("A warm feeling fills your body.\n\r", ch);
    update_pos( ch );

    if (IS_OAFFECTED(ch, AFF_GHOST))
    {
	send_to_char("Your ghostly magic fails to affect the material plane.\n\r", ch);
	return TRUE;
    }

    for (victim = ch->in_room->people; victim != NULL; victim = victim->next_in_room)
    {
        if (!is_same_group(ch, victim) || ch == victim)
            continue;

        if (is_an_avatar(victim))
            continue;

    	victim->hit = UMIN( victim->hit + 100, victim->max_hit );
        update_pos( victim );
        send_to_char( "A warm feeling fills your body.\n\r", victim );
    }
    if ( ch->leader || ch->master || ch->pet )
        send_to_char( "You extend the healing to your companions.\n\r", ch );

    return TRUE;
}

// need to add a mount check here

bool spell_meldwithwater( int sn, int level, CHAR_DATA *ch, void *vo,int target
)
{
    ROOM_INDEX_DATA *dest_room;
    CHAR_DATA *victim;
    CHAR_DATA *vch, *vch_next;

    if ( ( victim = get_char_world( ch, target_name ) ) == NULL
    ||   victim == ch
    ||   victim->in_room == NULL
    ||   is_symbol_present(*victim->in_room, OBJ_VNUM_SYMBOL_PROTECT) 
    ||   is_affected(ch, gsn_matrix)
    ||   !ch->in_room
    ||   (victim->in_room->sector_type != SECT_WATER_SWIM &&
                victim->in_room->sector_type != SECT_WATER_NOSWIM &&
                victim->in_room->sector_type != SECT_UNDERWATER)
    ||   (ch->in_room->sector_type != SECT_WATER_SWIM &&
                ch->in_room->sector_type != SECT_WATER_NOSWIM &&
                ch->in_room->sector_type != SECT_UNDERWATER &&
		!IS_SET(ch->in_room->room_flags, ROOM_HAS_WATER))
    ||   !can_see_room(ch,victim->in_room)
    ||   IS_SET(victim->in_room->room_flags, ROOM_NOGATE)
    ||   (victim->in_room->area->area_flags & AREA_UNCOMPLETE )
    ||   IS_SET(victim->in_room->room_flags, ROOM_NOMAGIC)
    ||   IS_SET(victim->in_room->room_flags, ROOM_SAFE)
    ||   IS_SET(victim->in_room->room_flags, ROOM_PRIVATE)
    ||   IS_SET(victim->in_room->room_flags, ROOM_SOLITARY)
    ||   IS_SET(victim->in_room->room_flags, ROOM_NO_RECALL)
    ||   IS_SET(ch->in_room->room_flags, ROOM_NO_RECALL)
    ||   (!IS_NPC(victim) && victim->level >= LEVEL_HERO)  /* NOT trust */
    ||   (IS_NPC(victim) && IS_SET(victim->imm_flags,IMM_SUMMON))
    ||   (ch->move == 0) )
    {
	send_to_char("You failed.\n\r", ch);
        return FALSE;
    }

    if (saves_spell(level, ch, victim, DAM_OTHER))
    {
	send_to_char("You failed.\n\r", ch);
	return TRUE;
    }

    dest_room = victim->in_room;

    for (vch = ch->in_room->people; vch != NULL; vch = vch_next)
    {
        vch_next = vch->next_in_room;
        if (is_same_group(vch, ch) && vch != ch)
        {
            act("$n suddenly turns translucent and melts away in a rush of water!", vch, NULL, NULL, TO_ROOM);
            act("You see your body turn translucent, and the world spins around you as the currents takes you!", vch, NULL, NULL, TO_CHAR);
            char_from_room(vch);
            char_to_room(vch, dest_room);
            do_look(vch, "auto");
            WAIT_STATE(vch, UMAX(vch->wait, 2*PULSE_VIOLENCE));
            act("A swirling current reforms into $n!", vch, NULL, NULL, TO_ROOM);
        }
    }

    ch->move /= 2;

    act("$n suddenly turns translucent and melts away in a rush of water!", ch, NULL, NULL, TO_ROOM);
    act("You see your body turn translucent, and the world spins around you as the currents takes you!", ch, NULL, NULL, TO_CHAR);
    char_from_room(ch);
    char_to_room(ch, dest_room);
    do_look(ch, "auto");
    WAIT_STATE(ch, UMAX(ch->wait, 2*PULSE_VIOLENCE));
    act("A swirling current reforms into $n!", ch, NULL, NULL, TO_ROOM);

    return TRUE;
}

bool spell_mendwounds( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    bool success = 0;

    if (!is_affected(victim, gsn_gouge) &&
	!is_affected(victim, gsn_grapple) &&
	!is_affected(victim, gsn_hamstring) &&
	!is_affected(victim, gsn_slice) &&
	!is_affected(victim, gsn_impale) &&
	!is_affected(victim, gsn_vitalstrike) &&
	!is_affected(victim, gsn_cleave) &&
	!is_affected(victim, gsn_boneshatter) &&
	!is_affected(victim, gsn_gash) &&
	!is_affected(victim, gsn_kneeshatter) &&
	!IS_SET(victim->oaffected_by, AFF_ONEHANDED))
    {
	if (ch != victim)
	    act("$N has no wounds that you can mend.", ch, NULL, victim, TO_CHAR);
	else if (!IS_AFFECTED(ch, AFF_REGENERATION))
	    act("You have no wounds to mend.", ch, NULL, NULL, TO_CHAR);
	return FALSE;
    }


    int minNum(20);
    if (is_affected(ch, gsn_physikersinstinct))
        minNum -= 5;

    if (is_affected(victim, gsn_gouge) && number_percent() > minNum)
    {
	success = 1;
        act("$n's eyes grow more clear.", victim, NULL, NULL, TO_ROOM);
        act("Your eyes grow clear as your gouging heals.", victim, NULL, NULL, TO_CHAR);
        affect_strip(victim, gsn_gouge);
    }

    if (is_affected(victim, gsn_grapple) && number_percent() > minNum)
    {
	success = 1;
        act("$n's limbs mend from their pained state.", victim, NULL, NULL, TO_ROOM);
        act("Your joints and muscles ease from their sprained, bruised state.", victim, NULL, NULL, TO_CHAR);
        affect_strip(victim, gsn_grapple);
    }

    if (is_affected(victim, gsn_hamstring) && number_percent() > minNum)
    {
	success = 1;
        act("$n's hamstring wound closes and heals.", victim, NULL, NULL, TO_ROOM);
        act("Your hamstring wound closes and heals.", victim, NULL, NULL, TO_CHAR);
        affect_strip(victim, gsn_hamstring);
    }

    if (is_affected(victim, gsn_slice) && number_percent() > minNum)
    {
	success = 1;
        act("$n's slice wound closes and heals.", victim, NULL, NULL, TO_ROOM);
        act("Your slice wound closes and heals.", victim, NULL, NULL, TO_CHAR);
        affect_strip(victim, gsn_slice);
    }

    if (is_affected(victim, gsn_impale) && number_percent() > minNum)
    {
	success = 1;
        act("$n's impalement wound closes up.", victim, NULL, NULL, TO_ROOM);
        act("Your impalement wound closes up.", victim, NULL, NULL, TO_CHAR);
        affect_strip(victim, gsn_impale);
    }

    if (is_affected(victim, gsn_vitalstrike) && number_percent() > minNum)
    {
	success = 1;
        act("$n's wounded vitals heal.", victim, NULL, NULL, TO_ROOM);
        act("Your wounded vital organs heal.", victim, NULL, NULL, TO_CHAR);
        affect_strip(victim, gsn_vitalstrike);
    }

    if (is_affected(victim, gsn_cleave) && number_percent() > minNum)
    {
	success = 1;
        act("$n's cleave wound closes up.", victim, NULL, NULL, TO_ROOM);
        act("Your cleave wound closes up.", victim, NULL, NULL, TO_CHAR);
        affect_strip(victim, gsn_cleave);
    }
    
    if (is_affected(victim, gsn_gash) && number_percent() > minNum)
    {
	success = 1;
        act("$n's gash wound closes up.", victim, NULL, NULL, TO_ROOM);
        act("Your gash wound closes up.", victim, NULL, NULL, TO_CHAR);
        affect_strip(victim, gsn_gash);
    }

    if (is_affected(victim, gsn_boneshatter) && number_percent() > minNum)
    {
	success = 1;
        if (IS_NAFFECTED(victim, AFF_ARMSHATTER))
        {
            act("$n's shattered arm heals.", victim, NULL, NULL, TO_ROOM);
            act("Your shattered arm heals.", victim, NULL, NULL, TO_CHAR);
        }
        if (IS_NAFFECTED(victim, AFF_LEGSHATTER))
        {
            act("$n's crushed leg heals.", victim, NULL, NULL, TO_ROOM);
            act("Your crushed leg heals.", victim, NULL, NULL, TO_CHAR);
        }
        affect_strip(victim, gsn_boneshatter);
    }

    if (IS_SET(victim->oaffected_by, AFF_ONEHANDED) && number_percent() > minNum)
    {
	success = 1;
	act("$n's missing hand is regenerated.", victim, NULL, NULL, TO_ROOM);
	act("Your missing hand is regenerated.", victim, NULL, NULL, TO_ROOM);
	REMOVE_BIT(victim->oaffected_by, AFF_ONEHANDED);
    }
    
    if (is_affected(victim, gsn_kneeshatter) && number_percent() > minNum)
    {
	success = 1;
        act("$n's shattered knees heal.", victim, NULL, NULL, TO_ROOM);
        act("Your shattered knees heal.", victim, NULL, NULL, TO_CHAR);
        affect_strip(victim, gsn_kneeshatter);
    }

    if(!success)
    {
	if (victim != ch)
	{
	    act("You failed to heal $N's wounds.", ch, NULL, victim, TO_CHAR);
	    act("$n failed to heal your wounds.", ch, NULL, victim, TO_VICT);
	}
	else if (!IS_AFFECTED(ch, AFF_REGENERATION))
	    act("You fail to heal your wounds.", ch, NULL, NULL, TO_CHAR);
    }

    return TRUE;
}


bool spell_protectionfromfire(int sn,int level,CHAR_DATA *ch,void *vo, int target)
{
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if (is_affected(ch, sn))
    {
        send_to_char("You are already protected from fire.\n\r", ch);
        return FALSE;
    }

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = 24;
    af.location  = APPLY_RESIST_FIRE;
    af.modifier  = 25;
    af.bitvector = 0;
    affect_to_char( ch, &af );
    send_to_char( "You feel a cool aura around you.\n\r", ch );
    act("A brief chill emanates from $n.",ch,NULL,NULL,TO_ROOM);
    return TRUE;
}


bool spell_protectionfrompoison(int sn,int level,CHAR_DATA *ch,void *vo, int target)
{
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if (is_affected(ch, sn))
    {
        send_to_char("You are already protected from foul poison.\n\r", ch);
        return FALSE;
    }

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = 24;
    af.location  = APPLY_RESIST_POISON;
    af.modifier  = 25;
    af.bitvector = 0;
    affect_to_char( ch, &af );
    send_to_char( "Your magic now protects you from poison.\n\r", ch );
    return TRUE;
}

static void check_healerstouch(CHAR_DATA * ch, CHAR_DATA * victim, int sn)
{
    // Check healers touch
    if (number_percent() > get_skill(ch, gsn_healerstouch))
    {
        check_improve(ch, NULL, gsn_healerstouch, false, 6);
        return;
    }

    // Skill check passed, apply the effect to the target
    check_improve(ch, NULL, gsn_healerstouch, true, 6);

    AFFECT_DATA af = {0};
    af.where    = TO_AFFECTS;
    af.type     = gsn_healerstouch;
    af.level    = ch->level;
    af.duration = ch->level / 12;
    af.modifier = sn;
    affect_to_char(victim, &af);

    send_to_char("You feel a faint warmth as the cleansing waters innoculate you.\n", victim);
    if (ch != victim)
        act("You impart $N with your healing touch, leaving $M more resistant than before.", ch, NULL, victim, TO_CHAR);
}

bool spell_purify( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;

    // Send echoes
	if (victim == ch)
	{
	    act("$n takes a sip of glowing water from $s cupped palm.",ch,NULL,NULL,TO_ROOM);
	    act("You take a sip of glowing water from your cupped palm.",ch,NULL,NULL,TO_CHAR);
	}
	else
    {
        act("$n offers $N a sip of glowing water.", ch, NULL, victim,TO_ROOM);
        act("$N sips cleansing waters from your hands.", ch, NULL, victim,TO_CHAR);
        act("You sip cleansing waters from the hands of $N.",victim, NULL, ch, TO_CHAR);
    }

    // Check malefic insight
    bool maleficCheck(false);
    if (number_percent() <= get_skill(ch, gsn_maleficinsight))
    {
        level += 10;
        maleficCheck = true;
    }

    if (is_affected(ch, gsn_physikersinstinct))
        level += 5;

    // Look to dispel poison and blindness
    bool dispelledAny(false);
    if (check_dispel(level, victim, gsn_poison))
    {
        act("$n looks much better.", victim, NULL, NULL, TO_ROOM);
        dispelledAny = true;
        check_healerstouch(ch, victim, gsn_poison);
    }
    
    if (check_dispel(level, victim, gsn_blindness))
    {
        act("$n marvels at $s new-found sight!", victim, NULL, NULL, TO_ROOM);
        dispelledAny = true;
        check_healerstouch(ch, victim, gsn_blindness);
    }
 
    // Check for boosts from treat infection
    if (number_percent() <= get_skill(ch, gsn_treatinfection))
    {
        if (check_dispel(level, victim, gsn_pox))
        {
            act("$n looks relieved as $s pox vanishes.", victim, NULL, NULL, TO_ROOM);
            dispelledAny = true;
            check_healerstouch(ch, victim, gsn_pox);
        }

        if (check_dispel(level, victim, gsn_fever))
        {
            act("$n looks relieved as $s fever vanishes.", victim,NULL,NULL,TO_ROOM);
            dispelledAny = true;
            check_healerstouch(ch, victim, gsn_fever);
        }

        if (check_dispel(level, victim, gsn_plague))
        {
            act("$n looks relieved as $s plague vanishes.", victim, NULL, NULL, TO_ROOM);
            dispelledAny = true;
            check_healerstouch(ch, victim, gsn_plague);
        }
 
        check_improve(ch, NULL, gsn_treatinfection, true, 6);
    }
    else
        check_improve(ch, NULL, gsn_treatinfection, false, 6);

    // Check for boosts from restore vigor
    if (number_percent() <= get_skill(ch, gsn_restorevigor))
    {
        if (check_dispel(level, victim, gsn_enfeeblement))
        {
            act("$n looks less feeble.", victim, NULL, NULL, TO_ROOM);
            dispelledAny = true;
            check_healerstouch(ch, victim, gsn_enfeeblement);
        }

        if (check_dispel(level, victim, gsn_weaken))
        {
            act("$n seems less weakened.", victim, NULL, NULL, TO_ROOM);
            dispelledAny = true;
            check_healerstouch(ch, victim, gsn_weaken);
        }

        if (check_dispel(level, victim, gsn_enervatingray))
        {
            act("$n seems less enervated.", victim, NULL, NULL, TO_ROOM);
            dispelledAny = true;
            check_healerstouch(ch, victim, gsn_enervatingray);
        }

        if (check_dispel(level, victim, gsn_heatwave))
        {
            act("$n appears less weary.", victim, NULL, NULL, TO_ROOM);
            dispelledAny = true;
            check_healerstouch(ch, victim, gsn_heatwave);
        }

        if (check_dispel(level, victim, gsn_weariness))
        {
            act("$n appears less weary.", victim, NULL, NULL, TO_ROOM);
            dispelledAny = true;
            check_healerstouch(ch, victim, gsn_weariness);
        }

        check_improve(ch, NULL, gsn_restorevigor, true, 6);
    }
    else
        check_improve(ch, NULL, gsn_restorevigor, false, 6);

    // Check for boosts from clarify mind
    if (number_percent() <= get_skill(ch, gsn_clarifymind))
    {
        if (check_dispel(level, victim, gsn_delusions))
        {
            if (ch != victim) act("$N no longer appears deluded.", ch, NULL, victim, TO_CHAR);
            dispelledAny = true;
            check_healerstouch(ch, victim, gsn_delusions);
        }

        if (check_dispel(level, victim, gsn_obfuscation))
        {
            if (ch != victim) act("$N shakes $S head as an obfuscating influence fades from $S vision.", ch, NULL, victim, TO_CHAR);
            act("$N shakes $S head as if to clear it.", ch, NULL, victim, TO_NOTVICT);
            dispelledAny = true;
            check_healerstouch(ch, victim, gsn_obfuscation);
        }

        if (check_dispel(level, victim, gsn_nightfears))
        {
            if (ch != victim) act("$N's nightmares subside under the power of your purifying magics.", ch, NULL, victim, TO_CHAR);
            dispelledAny = true;
            check_healerstouch(ch, victim, gsn_nightfears);
        }

        if (check_dispel(level, victim, gsn_powerwordfear))
        {
            if (ch != victim) act("$N's unfounded fear is banished.", ch, NULL, victim, TO_CHAR);
            dispelledAny = true; 
            check_healerstouch(ch, victim, gsn_powerwordfear);
        }

        if (check_dispel(level, victim, gsn_seedofmadness))
        {
            if (ch != victim) act("$N's sanity is restored.", ch, NULL, victim, TO_CHAR);
            dispelledAny = true;
            check_healerstouch(ch, victim, gsn_seedofmadness);
        }

        if (check_dispel(level, victim, gsn_visions))
        {
            if (ch != victim) act("The visions of damnation chasing $N vanish.", ch, NULL, victim, TO_CHAR);
            dispelledAny = true;
            check_healerstouch(ch, victim, gsn_visions);
        }

        CHAR_DATA * cloaker(cloak_remove(victim));
        if (cloaker != NULL)
        {
            send_to_char("You sense your cloak be forced away.\n", cloaker);
            send_to_char("Your mind clears, and you realize someone was cloaked from your sight.\n", victim);
            if (ch != victim) act("You clear $N's mind with your spell, and realize someone was cloaked from $S sight.", ch, NULL, victim, TO_CHAR);
            dispelledAny = true;
        }
    }

    // Check for improvement to malefic insight
    if (dispelledAny)
        check_improve(ch, NULL, gsn_maleficinsight, maleficCheck, 6);

    return true;
}

bool spell_rangedhealing( int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim;
    char arg[MAX_INPUT_LENGTH];
    char *direction;
    ROOM_INDEX_DATA *pRoom = NULL;
    int dir = -1, heal, i;

    if (!ch->in_room)
	return FALSE;

    direction = one_argument(target_name, arg);

    /* Target's name stored in variable 'target', direction of spell stored
       in target 'direction'*/

    if (arg[0] == '\0') 
    {
	send_to_char("Who are you trying to heal?\n\r", ch);
	return FALSE;
    }

    if (direction[0] == '\0')
        pRoom = ch->in_room;
    else
    {
	if (ch->fighting)
	{
	    send_to_char("You can't heal someone in another room while fighting!\n\r", ch);
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
            send_to_char("You can't see anyone in that direction!\n\r",ch);
            return FALSE;
        }

	pRoom = ch->in_room->exit[dir]->u1.to_room;
    }

    /* Ok, now we've found and set the location.  Now, if location is NULL,
       we know that the direction indicated does not exist. */

    if (pRoom == NULL)
    {
        send_to_char("You cannot heal someone in that direction!\n\r",ch);
        return FALSE;
    }

    if (room_is_affected(ch->in_room, gsn_smoke)  
     || room_is_affected(pRoom, gsn_smoke))
    {
        send_to_char("The thick smoke wafting by blocks you from targetting your healing.\n\r",ch);
        return FALSE;
    }

    if (pRoom == ch->in_room)
    {
    	if ((victim = get_char_room(ch,arg)) == NULL)
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

    if (victim == ch)
    {
	send_to_char("You cannot used ranged healing on yourself.\n\r", ch);
	return FALSE;
    }
    
    if (is_an_avatar(victim))
    {
        send_to_char("An avatar cannot receive such magic.\n\r",ch);
	    return FALSE;
    }

    heal = 30 + level/2;

    act("You raise your palm towards $N, filling $M with healing magic.", ch, NULL, victim, TO_CHAR);

    if (pRoom == ch->in_room)
    {
	act("$n raises $s palm towards $N.", ch, NULL, victim, TO_CHAR);
	act("$n raises $s palm towards you, filling you with healing magic.", ch, NULL, victim, TO_VICT);
    }
    else
    {
	char buf[MAX_STRING_LENGTH];

	sprintf(buf, "$n %s, and $s hand shimmers momentarily.",
		(dir == 0 ? "raises $s palm towards the north" 
		: dir == 1 ? "raises $s palm towards the east"
		: dir == 2 ? "raises $s palm towards the south"
		: dir == 3 ? "raises $s palm towards the west"
		: dir == 4 ? "raises $s palm upwards"
		: dir == 5 ? "lowers $s palm downward" : "concentrates"));
	act(buf, ch, NULL, NULL, TO_ROOM); 
    }

    act("$n looks better.", victim, NULL, NULL, TO_ROOM);
    act("You feel better!", victim, NULL, NULL, TO_CHAR);

    victim->hit = UMIN(victim->max_hit, victim->hit + heal);

    return TRUE;
}


bool spell_rebirth(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if (!is_same_group(ch, victim))
    {
        send_to_char("This spell may only be cast on groupmates.\n\r", ch);
        return FALSE;
    }

    if (is_affected(victim, gsn_rebirth))
    {
        send_to_char("They were recently reborn already.\n\r", ch);
        return FALSE;
    }

    if (checkLogorPreventHealing(ch, victim))
        return true;

    int dummy(0);
    checkMartyrsFire(ch, victim, dummy);

    act ("$n kneels as healing waters are sprinkled upon $s head!", (CHAR_DATA*)vo, NULL, NULL, TO_ROOM);
    send_to_char ("The healing water restores you.\n\r", (CHAR_DATA*)vo);

    victim->hit = victim->max_hit;
    victim->mana /= 2;
    victim->move = victim->max_move;

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = 24;
    af.modifier  = -100;
    af.location  = APPLY_HIT;
    af.bitvector = 0;
    affect_to_char( victim, &af );
    af.modifier  = -1;
    af.location  = APPLY_STR;
    affect_to_char( victim, &af );
    send_to_char( "You feel drained as you pour the healing waters.\n\r", ch );
    return TRUE;
}

bool spell_refresh( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    victim->move = UMIN(victim->move + UMAX(level,30), victim->max_move );
    if (victim->max_move == victim->move)
        send_to_char("You feel fully refreshed!\n\r",victim);
    else
        send_to_char( "You feel less tired.\n\r", victim );
    if ( ch != victim )
        act("$N looks less tired.", ch, NULL, victim, TO_CHAR );
    return TRUE;
}


bool spell_resurrection( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    OBJ_DATA *corpse;
    bool found=FALSE;
    OBJ_DATA *obj, *obj_next;

    if (!victim)
    {
        send_to_char("Resurrect whom?\n\r",ch);
	return FALSE;
    }

    if (IS_NPC(victim))
    {
	send_to_char("Their lifeforce is not strong enough to rekindle.\n\r", ch);
	return FALSE;
    }

    if  (!IS_OAFFECTED(victim, AFF_GHOST))
    {
        send_to_char("You can only resurrect a person who is still in ghostly form.\n\r", ch);
        return FALSE;
    }

    if (is_affected(victim, gsn_incineration))
    {
        send_to_char("The incineration which killed them has severed all connection between their body and spirit, and you fail.\n\r", ch);
        return FALSE;
    }
 
    if (victim == ch)
    {
	send_to_char("You are not able to resurrect yourself.\n\r", ch);
	return FALSE;
    }

    if (ch->in_room && area_is_affected(ch->in_room->area, gsn_deathwalk))
    {
	send_to_char("The grip of death is too strong to resurrect this corpse.\n\r", ch);
	return FALSE;
    }

    if (victim->pcdata->age_group == AGE_DEAD)
    {
	send_to_char("Their lifeforce is too weak to resurrect.\n\r", ch);
	return FALSE;
    }

    for (corpse = ch->in_room->contents; corpse != NULL; corpse = corpse->next_content)
    {
        if (corpse->item_type != ITEM_CORPSE_PC)
            continue;
        if (str_cmp(corpse->owner, victim->name))
            continue;
        found = TRUE;
        break;
    }

    if (!found)
    {
	act("You utter a word of power, but cannot find $N's corpse to merge $M with.", ch, NULL, victim, TO_CHAR);
        act("$n utters a word of power, but cannot find your corpse to merge you with.", ch, NULL, victim, TO_VICT);
        act("$n utters a word of power, but cannot find $N's corpse to merge $M with.", ch, NULL, victim, TO_NOTVICT);
        return FALSE;
    }
    
    act("As $n utters a word of power, the spirit of $N is sucked back into $S corpse, and $E gasps a shocked breath as $E returns to the living.", ch, NULL, victim, TO_NOTVICT);
    act("You utter a word of power, and the spirit of $N is sucked back into $S corpse, and $E gasps a shocked breath as $E returns to the living.", ch, NULL, victim, TO_CHAR);
    act("As $n utters a word of power, the world spins around you, and you find yourself gasping for breath, returned to your body.", ch, NULL, victim, TO_VICT);

    act("You feel some of your vitality return as you regain control of your body.", victim, NULL, NULL, TO_CHAR);
    victim->pcdata->death_count--;
    char buf[MAX_STRING_LENGTH];
    sprintf(buf,"%s resurrected %s in room %d.",IS_NPC(ch) ? ch->short_descr : ch->name,victim->name,ch->in_room->vnum);
    wiznet(buf,victim,NULL,WIZ_DEATHS,0,0);
    log_string(buf);

    for (obj = corpse->contains; obj != NULL; obj = obj_next)
    {
        obj_next = obj->next_content;
        obj_from_obj(obj);

	if (obj->item_type == ITEM_MONEY)
	{
	    coins_to_char(victim, obj->value[0], obj->value[1]);
	    extract_obj(obj);
	    continue;
	}

        obj_to_char(obj, victim);
    }

    affect_strip(victim, gsn_ghost);
    save_char_obj(victim);
    extract_obj(corpse);
    return TRUE;
}

bool spell_revitalize( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;

    if (checkLogorPreventHealing(ch, victim))
        return true;

    int heal = 200;
    checkMartyrsFire(ch, victim, heal);
    checkPhysickersInstinct(ch, heal);
    checkRevenant(*victim, heal);
    victim->hit = UMIN( victim->hit + heal, victim->max_hit );
    update_pos( victim );
    send_to_char( "A warm feeling surges through your body.\n\r", victim );
    if ( ch != victim )
        if(IS_NPC(victim))
	    act("You revitalize $N.",ch,NULL,victim,TO_CHAR);
	else
	{
            send_to_char("After your healing, ",ch);
	    if(!is_affected(victim,gsn_berserk))
	        show_char_status(victim,ch);
	    else
	        act("some of $N's wounds close.",ch,NULL,victim,TO_CHAR);
        }	    
    return TRUE;
}


bool spell_runeoflife( int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    OBJ_DATA *weapon;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    weapon = (OBJ_DATA *) vo;

    if (weapon == NULL)
    {
        send_to_char("You can't find any such weapon.\n\r", ch);
        return FALSE;
    }

    if (is_affected(ch, sn))
    {
        send_to_char("You already have added a rune of life recently.\n\r", ch);
        return FALSE;
    }

    if (obj_is_affected(weapon, sn))
    {
        send_to_char("That weapon already has a rune of life.\n\r", ch);
        return FALSE;
    }

    if (weapon->item_type != ITEM_WEAPON)
    {
        send_to_char("You can only apply a rune of life to a weapon.\n\r", ch);
        return FALSE;
    }

    af.where     = TO_OBJECT;
    af.type      = sn;
    af.level     = level;
    af.duration  = 70;
    af.location  = APPLY_MANA;
    af.modifier  = -50;
    af.bitvector = 0;
    affect_to_obj(weapon, &af);

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = 35;
    af.location  = 0;
    af.modifier  = 0;
    af.bitvector = 0;
    affect_to_char(ch, &af);

    act("$n chants softly, branding $p with a Rune of Life.", ch, weapon, NULL, TO_ROOM);
    act("You chant softly, branding $p with a Rune of Life.", ch, weapon, NULL, TO_CHAR);

    return TRUE;

}

bool spell_sanctuary( int sn, int level, CHAR_DATA *ch, void *vo,int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if ( IS_AFFECTED(victim, AFF_SANCTUARY) )
    {
        if (victim == ch)
          send_to_char("You are already in sanctuary.\n\r",ch);
        else
          act("$N is already in sanctuary.",ch,NULL,victim,TO_CHAR);
        return FALSE;
    }

    if (is_an_avatar(victim))
    {
        send_to_char("The magic cannot surround their holy form.\n\r", ch);
        return FALSE;
    }

    if (is_affected(victim, gsn_cloakofthevoid))
    {
	send_to_char("The cloak of the void prevents them from gaining sanctuary.\n\r", ch);
	return FALSE;
    }

    AFFECT_DATA * logorAff(get_affect(victim, gsn_ashesoflogor));
    if (logorAff != NULL && logorAff->modifier == 1)
    {
        if (victim == ch) send_to_char("Your magic cannot overcome the defilement within you.\n", ch);
        else act("Your magic cannot overcome the defilement within $M.", ch, NULL, victim, TO_CHAR);
        return false;
    }

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = level / 6;
    af.location  = APPLY_NONE;
    af.modifier  = 0;
    af.bitvector = AFF_SANCTUARY;
    affect_to_char( victim, &af );
    act( "$n is surrounded by a white aura.", victim, NULL, NULL, TO_ROOM );
    send_to_char( "You are surrounded by a white aura.\n\r", victim );
    return TRUE;
}


bool spell_scry( int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim;
    OBJ_DATA *obj;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    for (obj = ch->in_room->contents; obj != NULL; obj = obj->next_content)
        if (obj->item_type == ITEM_FOUNTAIN)
                break;

    if (obj == NULL || obj->item_type != ITEM_FOUNTAIN)
    {
        send_to_char("You can't find a fountain to scry in.\n\r", ch);
        return FALSE;
    }

    act("$n touches the surface of $p, which shimmers briefly.", ch, obj, NULL, TO_ROOM);
    act("You touch the surface of $p, which shimmers briefly.", ch, obj, NULL, TO_CHAR);

    if (( victim = get_char_world(ch, target_name)) == NULL)
    {
        act("No image appears, and you conclude you were unable to find the person you sought.", ch, NULL, NULL, TO_CHAR);
        return TRUE;
    }

    /* Scrying immortals?  I don't think so.  - Erinos */
    if (IS_IMMORTAL(victim))
    {
        act("$N's divine power prevents you from scrying $M.", ch, NULL, victim, TO_CHAR);
        return TRUE;
    }

    if (victim->in_room != NULL && is_symbol_present(*victim->in_room, OBJ_VNUM_SYMBOL_PROTECT))
    {
        act("$N seems to be warded against your divinations.", ch, NULL, victim, TO_CHAR);
        return true;
    }

    af.where	 = TO_AFFECTS;
    af.type	 = sn;
    af.level	 = level;
    af.duration  = 0;
    af.modifier  = 0;
    af.location  = APPLY_HIDE;
    af.bitvector = 0;
    affect_to_char(ch, &af);

    show_char_to_char_1(victim, ch);

    affect_strip(ch, sn);

    return TRUE;

}

bool spell_wallofwater( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    // Check for cooldown
    if (is_affected(ch, sn))
    {
    	send_to_char("You don't feel ready to summon another wall of water yet.\n", ch);
	    return false;
    }

    // Room checks
    if (ch->in_room == NULL || ch->in_room->sector_type == SECT_AIR)
    {
    	send_to_char("You cannot summon a wall of water here.\n", ch);
    	return false;
    }

    if (ch->in_room->sector_type == SECT_UNDERWATER)
    {
    	send_to_char("This place is already filled with water.\n", ch);
    	return false;
    }

    // Fill the room
    act("A massive wall of water gushes out of the ground, filling the area!", ch, NULL, NULL, TO_CHAR);
    act("A massive wall of water gushes out of the ground, filling the area!", ch, NULL, NULL, TO_ROOM);
    fillRoomWithWater(sn, level, level / 12, true, ch->in_room);

    // Apply cooldown
    AFFECT_DATA af = {0};
    af.where	= TO_AFFECTS;
    af.type	    = sn;
    af.level	= level;
    af.location	= APPLY_NONE;
    af.duration = 12;
    affect_to_char(ch, &af);

    return true;
}

CHAR_DATA * make_water_elemental(int level)
{
    CHAR_DATA * elemental(create_mobile(get_mob_index(MOB_VNUM_WATER_ELEMENTAL)));
    elemental->level = level;
    elemental->damroll = level / 2;
    elemental->hitroll = (level * 2) / 3;
    elemental->damage[0] = level / 2;
    elemental->damage[1] = 4;
    elemental->damage[2] = (level * 2) / 15;
    elemental->hit  = (level * 20) + number_range(1, 250);
    elemental->max_hit = elemental->hit;
    return elemental;
}

bool spell_waterelementalsummon( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
    if (is_affected(ch, gsn_waterelementalsummon))
    {
        send_to_char("You don't feel you can summon another water elemental yet.\n\r", ch);
        return FALSE;
    }

    if (ch->pet != NULL)
    {
        send_to_char("You already have a loyal follower, and cannot bind more loyalties.\n", ch);
        return FALSE;
    }

    if (ch->in_room == NULL || !is_water_room(*ch->in_room))
    {
        send_to_char("There isn't enough water here to summon a water elemental.\n\r", ch);
        return FALSE;
    }

    CHAR_DATA * elemental = make_water_elemental(level);
    char_to_room(elemental, ch->in_room);
    ch->pet = elemental;
    elemental->master = ch;
    elemental->leader = ch;

    act("Water swirls in violent currents, solidifying into a translucent water elemental, which bows to $n!", ch, NULL, NULL, TO_ROOM);
    act("Water swirls in violent currents, solidifying into a translucent water elemental, which bows to you!", ch, NULL, NULL, TO_CHAR);

    AFFECT_DATA af = {0};
    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = 40;
    affect_to_char(ch, &af);

    return TRUE;
}

bool spell_waterwalk( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if (is_affected(victim, sn))
    {
        if (victim == ch)
            send_to_char("You can already walk on water.\n\r",ch);
        else
            act("$N can already walk on water.",ch,NULL,victim,TO_CHAR);
        return FALSE;
    }

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = level;
    af.modifier  = 0;
    af.location  = 0;
    af.bitvector = 0;
    affect_to_char( victim, &af );

    send_to_char( "You suddenly feel incredibly buouyant, as though you could walk on water.\n\r", victim );
    if ( ch != victim )
        act("$N can now walk on water.",ch,NULL,victim,TO_CHAR);

    return TRUE;
}

bool spell_whirlpool( int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *)vo;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if (ch->in_room == NULL)
        return FALSE;

    if (is_affected(victim, sn))
    {
        act("$N is already caught in a whirlpool.", ch, NULL, victim, TO_CHAR);
        return false;
    }

    if (ch->in_room->sector_type != SECT_WATER_SWIM && ch->in_room->sector_type != SECT_WATER_NOSWIM && ch->in_room->sector_type != SECT_UNDERWATER)
    {
        send_to_char("There is no water for a whirlpool here.\n\r", ch);
        return FALSE;
    }

    act ("$n calls a raging whirlpool to destroy you!", ch, NULL, victim, TO_VICT);
    act ("You call a raging whirlpool to destroy $N!", ch, NULL, victim, TO_CHAR);
    act ("$n calls a raging whirlpool to destroy $N!", ch, NULL, victim, TO_NOTVICT);

    if ( saves_spell( level, ch, victim, DAM_DROWNING ) )
    {
        act ("You avoid the whirlpool!", victim, NULL, NULL, TO_CHAR);
        act ("$n avoids the raging whirlpool!", victim, NULL, NULL, TO_ROOM);
        return TRUE;
    }

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = 1;
    af.location  = 0;
    af.modifier  = 0;
    af.bitvector = 0;
    affect_to_char(victim, &af);

    act("$n is caught in the raging whirlpool!", victim, NULL, NULL, TO_ROOM);
    act("You are caught in the raging whirlpool!", victim, NULL, NULL, TO_CHAR);
    
    return TRUE;
}

void destroy_icyprison(ROOM_INDEX_DATA *pRoomIndex, ROOM_INDEX_DATA *tRoom)
{
    AFFECT_DATA *paf;
    CHAR_DATA *vch, *vch_next;
    OBJ_DATA *pObj, *obj_next;

    if (tRoom)
    {
        for (vch = pRoomIndex->people; vch; vch = vch_next)
        {
            vch_next = vch->next_in_room;
	    global_linked_move = TRUE;
            char_from_room(vch);
            char_to_room(vch, tRoom);
            if (tRoom->vnum != 0)
                vch->was_in_room = NULL;
        }

     	for (pObj = tRoom->contents; pObj; pObj = obj_next)
     	{
            obj_next = pObj->next_content;
            if (pObj->pIndexData->vnum == OBJ_VNUM_ICY_PRISON)
            {
		for (paf = pObj->affected; paf; paf = paf->next)
		    if ((paf->type == gsn_icyprison) && (paf->point == (void *) pRoomIndex))
		    {
                	extract_obj(pObj);
	        	break;
		    }
            }
        }

     	for (pObj = pRoomIndex->contents; pObj; pObj = obj_next)
     	{
	    obj_next = pObj->next_content;
            obj_from_room(pObj);
            obj_to_room(pObj, tRoom);
     	}

     	act("An icy prison melts away.", tRoom->people, NULL, NULL, TO_CHAR);
     	act("An icy prison melts away.", tRoom->people, NULL, NULL, TO_ROOM);
    }
    else
    {
        ROOM_INDEX_DATA *recall = get_room_index(ROOM_VNUM_TEMPLE);
        for (vch = pRoomIndex->people; vch; vch = vch_next)
        {
            vch_next = vch->next_in_room;

            char_from_room(vch);
            char_to_room(vch, recall);
            if (tRoom->vnum != 0)
                 vch->was_in_room = NULL;

            send_to_char("Something seems amiss...\n\r", vch);
        }
    }
    free_room_area(pRoomIndex);
}
        

bool spell_douse( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;

    if (!IS_AFFECTED(ch, AFF_WIZI))
    {
        if (victim == ch)
        {
            act("$n gestures, and is covered by glowing water.",ch,NULL,NULL,TO_ROOM);
            act("You gesture, and are covered by glowing water.",ch,NULL,NULL,TO_CHAR);
        }
        else
        {
            act("$n gestures, and $N is covered by glowing water.", ch, NULL, victim,TO_NOTVICT);
	    act("You gesture, and $N is covered by glowing water.", ch, NULL, victim,TO_CHAR);
            act("$n gestures, and you are covered by glowing water.",ch, NULL, victim, TO_VICT);
        }
    }

    if (is_affected(ch, gsn_physikersinstinct))
        level += 5;

    if (check_dispel(level, victim, gsn_consume))
    {
        send_to_char("You stop sweating so profusely as the consuming fires are doused.\n", victim);
        act("$n stops sweating profusely.",victim,NULL,NULL,TO_ROOM);
    }
    if (check_dispel(level, victim, gsn_ignite))
    {
        send_to_char("The flames about you go out.\n", victim);
        act("The flames about $n go out.",victim,NULL,NULL,TO_ROOM);
    }
    if (check_dispel(level, victim, gsn_aggravatewounds))
    {
        send_to_char("Your burning wounds cool and close.\n", victim);
        act("$n's burning wounds cool.",victim,NULL,NULL,TO_ROOM);
    }
    if (check_dispel(level, victim, gsn_smolder))
    {
        send_to_char("You grow more comfortable as you stop smoldering.\n", victim);
        act("$n looks more comfortable.",victim,NULL,NULL,TO_ROOM);
    }

    return TRUE;
}

void do_healingward(CHAR_DATA *ch, char *argument)
{
AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
CHAR_DATA *vch;
int chance;
OBJ_DATA *shield;
    if ((chance = get_skill(ch, gsn_healingward)) == 0)
    {
        send_to_char("Huh?\n\r", ch);
        return;
    }

    if (ch->fighting == NULL)
    {
	send_to_char("You're not fighting anyone.\n\r", ch);
	return;
    }

    if (is_affected(ch, gsn_healingward))
    {
	send_to_char("You do not feel ready to cast another healing ward yet.\n\r", ch);
	return;
    }

    if ((shield = get_eq_char(ch,WEAR_SHIELD)) == NULL)
    {
	send_to_char("You must be wearing a life shield to cast a healing ward.\n\r",ch);
	return;
    }

    if ( number_percent() > chance)
    {
	act("$n tries to cast a healing ward, but is distracted and fails.",ch, NULL, NULL, TO_ROOM);
	act("You try to cast a healing ward, but you are distracted and fail.", ch, NULL, NULL, TO_CHAR);
        check_improve(ch,NULL,gsn_healingward,FALSE,1);
	WAIT_STATE(ch, UMAX(ch->wait, skill_table[gsn_healingward].beats));
	return;
    }
    if (ch->battlecry != NULL)
	do_yell(ch, ch->battlecry);
    else
	do_yell(ch, "For the Light!");

    act("$n glows for a moment, and a healing ward surrounds $m!",
	ch, NULL, NULL, TO_ROOM);
    act("You glow for a moment, and a healing ward surrounds you!",
	ch, NULL, NULL, TO_CHAR);
    
    af.where        = TO_NAFFECTS;
    af.type         = gsn_healingward;
    af.level        = ch->level;
    af.duration     = 6;
    af.location     = 0;
    af.bitvector    = AFF_RALLY;
    af.modifier     = 0;
    affect_to_char(ch, &af);

    for (vch = ch->in_room->people;vch != NULL;vch = vch->next_in_room)
    {
        if (is_same_group(ch, vch))
        {
            if(is_an_avatar(vch))
                continue;
            vch->hit = UMIN(vch->max_hit, vch->hit+vch->level*2);
        }
    }

    WAIT_STATE(ch, UMAX(ch->wait, skill_table[gsn_healingward].beats));
    check_improve(ch,NULL,gsn_healingward,TRUE,1);
}

void do_froststrike(CHAR_DATA *ch, char *argument )
{
    OBJ_DATA *obj;
    CHAR_DATA *victim;
    int chance;
    bool in_form = FALSE;

    if ((chance = get_skill(ch, gsn_froststrike)) ==0)
    {
	send_to_char("Huh?\n\r", ch);
	return;
    }

    if ( ( victim = ch->fighting) == NULL)
    {
	send_to_char("You aren't fighting anyone.\n\r", ch);
	return;
    }
    
    if ( ( obj = get_eq_char(ch,WEAR_WIELD)) == NULL)
    {
	send_to_char("You must wield a weapon to perform a frost strike.\n\r",ch);
	return;
    }

    if (obj->value[0] != WEAPON_SWORD)
    {
	send_to_char("You must wield a sword to perform a frost strike.\n\r",ch);
	return;
    }

    if (ch->mana - skill_table[gsn_froststrike].min_mana < 0)
    {
	send_to_char("You are too tired to perform a frost strike.\n\r",ch);
	return;
    }

    if (!check_reach(ch, victim, REACH_NORMAL, PULSE_VIOLENCE))
	return;
    AFFECT_DATA *paf;
    chance *= .75;

    for ( paf = ch->affected; paf; paf=paf->next)
	if (paf->location == APPLY_FORM)
	{
	    in_form = TRUE;
	    break;
	}
    if (paf)
    {
	chance *= 1.2;
	WAIT_STATE(ch, skill_table[gsn_froststrike].beats);
    }
    else
	WAIT_STATE(ch, skill_table[gsn_froststrike].beats * 2);

    expend_mana(ch, skill_table[gsn_froststrike].min_mana);

    int thac0, thac0_00, thac0_32, victim_ac, dam, diceroll, skill;
    int dam_type = DAM_SOUND;
    if (is_an_avatar(ch))
	    dam_type = TYPE_HIT+DAM_HOLY;
    if(obj && obj_is_affected(obj,gsn_heatsink))
	damage_old(ch, ch, number_range(4,8), gsn_heatsink,DAM_COLD,TRUE);
    wrathkyana_combat_effect(ch, victim);
    skill = get_skill(ch,gsn_froststrike);

    if (IS_NPC(ch))
    {
	thac0_00 = 20;
	thac0_32 = -4;
	if (IS_SET(ch->act,ACT_WARRIOR))
	    thac0_32 = -10;
	else if (IS_SET(ch->act,ACT_THIEF))
	    thac0_32 = -4;
    }
    else
    {
	thac0_00 = class_table[ch->class_num].thac0_00;
	thac0_32 = class_table[ch->class_num].thac0_32;
    }
    thac0 = interpolate(ch->level, thac0_00,thac0_32);
    if (thac0 < 0)
	thac0 = thac0 / 2;
    if (thac0 < -5)
	thac0 = -5 + (thac0+5)/2;
    thac0 -= GET_HITROLL(ch) * skill/100;
    thac0 += 5 * (100 - skill) / 100;
    victim_ac = GET_AC(victim,AC_EXOTIC)/10;
    
    if (victim_ac < -15)
	victim_ac = (victim_ac + 15) / 5 - 15;
    if (!can_see(ch,victim) && !is_blindfighter(ch, TRUE))
	victim_ac -= 4;
    if (victim->position < POS_FIGHTING)
	victim_ac += 4;
    if (victim->position < POS_RESTING)
	victim_ac += 6;
    if (victim->position > POS_RESTING)
	if (number_percent() < get_skill(victim,gsn_dodge))
	    victim_ac -= 4;
	if (number_percent() < get_skill(victim,gsn_evade))
	    victim_ac -= 4;

    while ((diceroll = number_bits(5) ) >= 20)
	;
    if ((diceroll == 0 || (diceroll != 19 && diceroll < thac0 - victim_ac)))
    {
	/* Miss */
	damage(ch,victim,0,gsn_froststrike,dam_type,TRUE);
	check_improve(ch,victim,gsn_froststrike,FALSE,1);
	return;
    }
    /* Hit */
    if (obj->pIndexData->new_format)
	dam = dice(obj->value[1], obj->value[2]);
    else
	dam = number_range(obj->value[1], obj->value[2]);
    dam *= 2;
    dam = (dam * skill)/100;
    dam += check_extra_damage(ch,dam,obj);
    if (!IS_AWAKE(victim))
	dam *= 2;
    dam += (GET_DAMROLL(ch) /3) * UMIN(100,skill) / 100;
    if (dam <= 0)
	dam = 1;
    check_improve(ch,victim,gsn_froststrike,TRUE,1);

    OBJ_DATA * vchWeapon(get_eq_char(victim, WEAR_WIELD));
    if (number_percent() <= skill/2 && vchWeapon != NULL && !IS_SET(vchWeapon->extra_flags[0], ITEM_VIS_DEATH) && !obj_is_affected(vchWeapon, gsn_heatsink) && !saves_spell(ch->level, ch, victim, DAM_COLD))
    {
        if (in_form)
        {
            act("You work a calculated strike into your form, sending chill energy into $p!",ch,vchWeapon,NULL,TO_CHAR);
            act("$n works a calculated strike into $s form, sending chill energy into $p!",ch,vchWeapon,NULL,TO_ROOM);
        }
        else
        {
            act("You strike with precision, sending chill energy into $p!",ch,vchWeapon,NULL,TO_CHAR);
            act("$n strikes with precision, sending chill energy into $p!",ch,vchWeapon,NULL,TO_ROOM);
        }
        AFFECT_DATA af = {0};
        af.where = TO_OBJECT;
        af.type = gsn_heatsink;
        af.level = ch->level;
        af.duration = 2;
        affect_to_obj(vchWeapon, &af);
    }
    else
    {
        if (in_form)
        {
            act("You work a calculated strike into your form, and a blue glow flickers down $p.",ch,obj,victim,TO_CHAR);
            act("$n works a calculated strike into $s form, and a blue glow flickers down $p.",ch,obj,victim,TO_ROOM);
        }
        else
        {
            act("You strike with precision, and a blue glow flickers down $p.",ch,obj,victim,TO_CHAR);
            act("$n strikes with precision, and a blue glow flickers down $p.",ch,obj,victim,TO_ROOM);
        }
    }
    damage_old(ch,victim,dam,gsn_froststrike,dam_type,TRUE);
    check_killer(ch,victim);
    return;
}

bool spell_cure_all_serious( int sn, int level, CHAR_DATA *ch, void *vo,int target )
// This is for Alajial's sigil
{
    CHAR_DATA *victim;
    int heal = 18 + level / 4;
    act("As you invoke your sigil, the area seem to brighten.",ch,NULL,NULL,TO_CHAR);
    act("As $n invokes $s sigil, the area seems to brighten.",ch,NULL,NULL,TO_ROOM);
    ch->hit = UMIN( ch->hit + heal, ch->max_hit );
    send_to_char("You feel better!\n\r", ch);
    update_pos( ch );

    if (IS_OAFFECTED(ch, AFF_GHOST))
    {
	send_to_char("Your ghostly magic fails to affect the material plane.\n\r", ch);
	return TRUE;
    }

    for (victim = ch->in_room->people; victim != NULL; victim = victim->next_in_room)
    {
    	if (victim == ch)
	        continue;
        
        if (is_an_avatar(victim))
            continue;

    	victim->hit = UMIN( victim->hit + heal, victim->max_hit );
        update_pos( victim );
        send_to_char("You feel better!\n\r", victim );
    }
    if ( ch->leader || ch->master || ch->pet )
        send_to_char( "You extend the healing to your companions.\n\r", ch );

    return TRUE;
}
