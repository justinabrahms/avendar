#include "PyreInfo.h"
#include "interp.h"
#include "tables.h"
#include "languages.h"
#include "spells_fire.h"
#include "spells_void.h"
#include "EchoAffect.h"
#include <sstream>
#include <iomanip>

PyreInfo::PyreInfo(OBJ_DATA * pyre) : 
    m_pyre(pyre),
    m_color(Orange),
    m_continueFun(NULL),
    m_step(0),
    m_lastIncineration(0),
    m_currSpellGreater(false),
    m_activePersistentSpell(Effect_None),
    m_activePersistentSpellGreater(false),
    m_effectModifier(0)
{}

const char * PyreInfo::effectName(Effect effectValue, bool capital)
{
    // Lookup the effect name
    switch (effectValue)
    {
        case Effect_None:                   return (capital ? "None" : "none");
        case Effect_EssenceOfTheInferno:    return (capital ? "Essence of the Inferno" : "essence of the inferno");
        case Effect_WreathOfFlame:          return (capital ? "Wreath of Flame" : "wreath of flame");
        case Effect_BrimstoneConduit:       return (capital ? "Brimstone Conduit" : "brimstone conduit");
        case Effect_BayyalsRealm:           return (capital ? "Bayyal's Realm" : "bayyal's realm");
        case Effect_Heatlash:               return (capital ? "Heatlash" : "heatlash");
        case Effect_FuryOfAdaChemtaBoghor:  return (capital ? "Fury of Ada'Chemta'Boghor" : "fury of ada'chemta'boghor");
        case Effect_Max:                    break;
    }

    // Did not find the name
    std::ostringstream mess;
    mess << "Did not find persistent effect in name lookup [Num: " << effectValue << "]";
    bug(mess.str().c_str(), 0);
    return (capital ? "Unknown" : "unknown");
}

void PyreInfo::addEffectIfKnown(std::ostringstream & result, CHAR_DATA * ch, unsigned int lesserBit, unsigned int greaterBit, const char * name)
{
    // Check for lesser
    if (lesserBit != Bit_None && !testBit(ch, lesserBit))
        return;
    
    // Has lesser, check for greater
    result << std::setw(27) << std::left << (std::string(name) + ':');
    if (greaterBit == Bit_None || testBit(ch, greaterBit)) result << "{Wgreater{x";
    else result << "{Wlesser{x";
    result << '\n';
}

std::string PyreInfo::listKnownEffects(CHAR_DATA * ch)
{
    std::ostringstream result;
    result << std::setfill(' ');
    addEffectIfKnown(result, ch, Bit_ReadEarendamScroll, Bit_SacrificedHeartOfInferno, effectName(Effect_WreathOfFlame, true));
    addEffectIfKnown(result, ch, Bit_None, Bit_StudiedWhipRune, effectName(Effect_FuryOfAdaChemtaBoghor, true));
    addEffectIfKnown(result, ch, Bit_ReadGogothScroll, Bit_XiganathAchievement, effectName(Effect_BayyalsRealm, true));
    addEffectIfKnown(result, ch, Bit_MergedWithInferno, Bit_EatenHeartOfKzroth, effectName(Effect_EssenceOfTheInferno, true));
    addEffectIfKnown(result, ch, Bit_HeldPhoenixFeather, Bit_EatenPhoenixEgg, effectName(Effect_Heatlash, true));
    addEffectIfKnown(result, ch, Bit_None, Bit_DefeatedDedicant, effectName(Effect_BrimstoneConduit, true));
    addEffectIfKnown(result, ch, Bit_SprinkledConsumePowder, Bit_None, "Inferno's Reach");
    addEffectIfKnown(result, ch, Bit_None, Bit_None, "Steelscald");
    addEffectIfKnown(result, ch, Bit_None, Bit_SsesarykCompleted, "Brimstone Ward");
    addEffectIfKnown(result, ch, Bit_None, Bit_None, "Burnt Offering");
    addEffectIfKnown(result, ch, Bit_None, Bit_None, "Flamekiss");
    addEffectIfKnown(result, ch, Bit_OajmaQuestCompleted, Bit_None, "Fireflies");
    addEffectIfKnown(result, ch, Bit_ReadGaaldScroll, Bit_None, "Flamecall");
    addEffectIfKnown(result, ch, Bit_None, Bit_ForbiddenBooksCompleted, "Flickerfyre");
    addEffectIfKnown(result, ch, Bit_StudiedLizardSkull, Bit_StaredGiantRuby, "Summon Salamander");
    addEffectIfKnown(result, ch, Bit_ReadGaaldScroll, Bit_None, "Trail of Embers");
    addEffectIfKnown(result, ch, Bit_None, Bit_None, "Hearthglow");
    return result.str();
}

bool PyreInfo::incinerateObject(CHAR_DATA * ch, OBJ_DATA * obj)
{
    if (m_color != Orange)
        m_lastIncineration = current_time;

    act("Murmuring an arcane word, you toss $p into the pyre.", ch, obj, NULL, TO_CHAR);
    act("Murmuring an arcane word, $n tosses $p into the pyre.", ch, obj, NULL, TO_ROOM);

    // Check for chance of minor failure, decreasing with skill
    if (number_percent() < UMIN(6, UMAX(2, 6 - (get_skill(ch, gsn_bloodpyre) - 80) / 5)) + (int)m_step)
    {
        // Failure hit
        send_to_char("You fail to focus your will on the flames, and sense the Inferno within shake briefly free of your control!\n", ch);
        act("The pyre flares brightly, sparking and hissing furiously as it disgorges $p with a blast of heat!", ch, obj, NULL, TO_CHAR);
        act("The pyre flares brightly, sparking and hissing furiously as it disgorges $p with a blast of heat!", ch, obj, NULL, TO_ROOM);
        
        // Check for room to throw the object into
        if (ch == obj->carried_by)
            obj_from_char(obj);

        ROOM_INDEX_DATA * targetRoom(NULL);
        int dir(0);
        if (number_percent() < 50)
        {
            for (unsigned int i(0); i < 10 && targetRoom == NULL; ++i)
            {
                dir = number_range(0, 5);
                EXIT_DATA * pexit(ch->in_room->exit[dir]);
                if (pexit != NULL && !IS_SET(pexit->exit_info, EX_CLOSED)) 
                    targetRoom = pexit->u1.to_room;
            }
        }
        
        if (targetRoom == NULL)
            targetRoom = ch->in_room;

        obj_to_room(obj, targetRoom);

        // Build the message
        if (targetRoom != ch->in_room)
        {
            std::ostringstream mess;
            mess << "$p flies through the air, landing somewhere out of sight";
            switch (dir)
            {
                case 0: mess <<" to the north"; break;
                case 1: mess <<" to the east"; break;
                case 2: mess <<" to the south"; break;
                case 3: mess <<" to the west"; break;
                case 4: mess <<" above you"; break;
                case 5: mess <<" below you"; break;
                default: bug("Invalid direction during incineration failure", 0); break;
            }

            // Echo the message
            mess << "!";
            act(mess.str().c_str(), ch, obj, NULL, TO_CHAR);
            act(mess.str().c_str(), ch, obj, NULL, TO_ROOM);

            // Also echo to the target room
            if (targetRoom->people != NULL)
            {
                act("$p suddenly flies into the vicinity suddenly, landing nearby with an unceremonious whoomph!", targetRoom->people, obj, NULL, TO_CHAR);
                act("$p suddenly flies into the vicinity suddenly, landing nearby with an unceremonious whoomph!", targetRoom->people, obj, NULL, TO_ROOM);
            }
        }
        
        sourcelessDamage(ch, "the pyre", dice(5, 50), gsn_bloodpyre, DAM_FIRE);
        send_to_char("When you finally wrest the pyre back under your control, you realize it has grown noticeably cooler.\n", ch);
        reset();
        return false;
    }

    // Check for a bloodstone, which can be used to reset the spell
    bool destroyObject(true);
    if (obj->item_type == ITEM_GEM && obj->material == material_lookup("bloodstone"))
    {
        act("The pyre flares brightly, sparking in a flurry of colors! After a moment, it settles down to a dull glow, the flame noticeably cooler.", ch, obj, NULL, TO_CHAR);
        act("The pyre flares brightly, sparking in a flurry of colors! After a moment, it settles down to a dull glow, the flame noticeably cooler.", ch, obj, NULL, TO_ROOM);
        reset();
        rawSetColor(ch, Orange, NULL);
        m_activePersistentSpell = Effect_None;
        m_activePersistentSpellGreater = false;
        m_effectModifier = 0;
    }
    else
    {
        // Check for a started spell
        if (m_continueFun != NULL) 
            (this->*m_continueFun)(ch, obj, destroyObject);
        else
        {
            // New spell; dispatch according to color
            switch (m_color)
            {
                case Orange:    handleOrange(ch, obj);  break;
                case Red:       handleRed(ch, obj);     break;
                case Blue:      handleBlue(ch, obj);    break;
                case White:     handleWhite(ch, obj);   break;
                default:
                {
                    std::ostringstream mess;
                    mess << "Unknown pyre color state " << m_color;
                    send_to_char("A problem has occurred; please contact the gods.\n", ch);
                    bug(mess.str().c_str(), 0);
                    return destroyObject;
                }
            }
        }
    }

    return destroyObject;
}

void PyreInfo::checkCooldownTime(CHAR_DATA * ch)
{
    // Ignore if no effect has been started
    if (m_lastIncineration == 0)
        return;

    // Check for a reset from timeout
    int pulsesSkipped((current_time - m_lastIncineration) / PULSE_VIOLENCE);
    if (number_percent() < pulsesSkipped * 2 + (int)m_step)
    {
        send_to_char("You sense your pyre grow cooler as it consumes the last lingering energies of the fuel you provided.\n", ch);
        reset();
    }
}

void PyreInfo::handleOrange(CHAR_DATA * ch, OBJ_DATA * obj)
{
    // When orange, only color changes are allowed; these require gems
    if (obj->item_type != ITEM_GEM)
    {
        handleIncorrect(ch, obj);
        return;
    }

    // Handle the material
    if (obj->material == material_lookup("ruby")) handleSetColor(ch, obj, Red, "red");
    else if (obj->material == material_lookup("sapphire")) handleSetColor(ch, obj, Blue, "blue");
    else if (obj->material == material_lookup("diamond")) handleSetColor(ch, obj, White, "white");
    else handleIncorrect(ch, obj);
}

void PyreInfo::handleRed(CHAR_DATA * ch, OBJ_DATA * obj)
{
    // Check all the possible spells starting with a red flame
    if (checkStart(ch, obj, true, Bit_ReadEarendamScroll, Bit_SacrificedHeartOfInferno, &PyreInfo::Proc_WreathOfFlames, NULL, ITEM_OIL, Req_None, Req_None)) return;
    if (checkStart(ch, obj, true, Bit_None, Bit_StudiedWhipRune, &PyreInfo::Proc_FuryOfAdaChemtaBoghor, "oak", ITEM_WEAPON, Req_None, Req_None)) return;
    if (checkStart(ch, obj, true, Bit_ReadGogothScroll, Bit_XiganathAchievement, &PyreInfo::Proc_BayyalsRealm, "smoke", Req_None, Req_None, Req_None)) return;
    if (checkStart(ch, obj, false, Bit_SprinkledConsumePowder, Bit_None, &PyreInfo::Proc_InfernosReach, "bone", Req_None, Req_None, Req_None)) return;
    if (checkStart(ch, obj, false, Bit_None, Bit_None, &PyreInfo::Proc_Steelscald, "steel", ITEM_ARMOR, Req_None, Req_None)) return;
    if (checkStart(ch, obj, false, Bit_None, Bit_SsesarykCompleted, &PyreInfo::Proc_Heatmine, "wax", Req_None, Req_None, Req_None)) return;

    // Invalid starting object
    handleIncorrect(ch, obj);
}

void PyreInfo::handleBlue(CHAR_DATA * ch, OBJ_DATA * obj)
{
    // Check all the possible spells starting with a blue flame
    if (checkStart(ch, obj, true, Bit_MergedWithInferno, Bit_EatenHeartOfKzroth, &PyreInfo::Proc_EssenceOfTheInferno, "smoke", Req_None, Req_None, Req_None)) return;
    if (checkStart(ch, obj, true, Bit_HeldPhoenixFeather, Bit_EatenPhoenixEgg, &PyreInfo::Proc_Heatlash, NULL, ITEM_STAFF, Req_None, Req_None)) return;
    if (checkStart(ch, obj, false, Bit_None, Bit_None, &PyreInfo::Proc_HungerOfTheFlame, "spice", ITEM_FOOD, Req_None, Req_None)) return;
    if (checkStart(ch, obj, false, Bit_None, Bit_None, &PyreInfo::Proc_Flamekiss, "crystal", Req_None, Req_None, Req_None)) return;

    // Invalid starting object
    handleIncorrect(ch, obj);
}

void PyreInfo::handleWhite(CHAR_DATA * ch, OBJ_DATA * obj)
{
    // Check all the possible spells starting with a white flame
    if (checkStart(ch, obj, true, Bit_None, Bit_DefeatedDedicant, &PyreInfo::Proc_BrimstoneConduit, NULL, ITEM_SCROLL, Req_None, Req_None)) return;
    if (checkStart(ch, obj, false, Bit_OajmaQuestCompleted, Bit_None, &PyreInfo::Proc_Fireflies, NULL, Req_None, OBJ_VNUM_TROPHY_EYE, 50)) return;
    if (checkStart(ch, obj, false, Bit_ReadGaaldScroll, Bit_None, &PyreInfo::Proc_Flamecall, NULL, Req_None, OBJ_VNUM_TROPHY_EAR, 50)) return;
    if (checkStart(ch, obj, false, Bit_None, Bit_ForbiddenBooksCompleted, &PyreInfo::Proc_FlickerFyre, "water", Req_None, Req_None, Req_None)) return;
    if (checkStart(ch, obj, false, Bit_StudiedLizardSkull, Bit_StaredGiantRuby, &PyreInfo::Proc_SummonSalamander, NULL, Req_None, OBJ_VNUM_TROPHY_FINGER, Req_None)) return;
    if (checkStart(ch, obj, false, Bit_ReadGaaldScroll, Bit_None, &PyreInfo::Proc_TrailOfEmbers, "sulphur", Req_None, Req_None, Req_None)) return;
    if (checkStart(ch, obj, false, Bit_None, Bit_None, &PyreInfo::Proc_Hearthglow, NULL, ITEM_LIGHT, Req_None, Req_None)) return;

    // Invalid starting object
    handleIncorrect(ch, obj);
}

void PyreInfo::reset()
{
    m_lastOwners.clear();
    m_continueFun = NULL;
    m_step = 0;
    m_lastIncineration = 0;
    m_currSpellGreater = false;
}

bool PyreInfo::checkStart(  CHAR_DATA * ch, OBJ_DATA * obj, bool persistent, unsigned int lesserPrereqBit, unsigned int greaterPrereqBit, 
                            ContinueFun continueFun, const char * materialName, 
                            unsigned int itemType, unsigned int vnum, unsigned int minLevel)
{
    // Make sure this char has data
    if (ch->pcdata == NULL || ch->pcdata->bitptr == NULL)
    {
        bug("Blood pyre caster missing bit data", 0);
        return false;
    }

    // Only one persistent effect, then the pyre is locked in
    if (persistent && m_activePersistentSpell != Effect_None)
        return false;

    // If the character does not meet the lesser prereq, this spell is not an option
    if (lesserPrereqBit != Bit_None && !testBit(ch, lesserPrereqBit))
        return false;

    // Determine whether this character has unlocked the greater version
    m_currSpellGreater = (greaterPrereqBit == Bit_None || testBit(ch, greaterPrereqBit));

    // Check whether the item matches the constraints
    if (!checkMatch(obj, materialName, itemType, vnum, minLevel))
        return false;

   // Item checks out; spell started
   handleCorrect(ch, obj);
   m_continueFun = continueFun;
   m_step = 0;
   return true;
}

bool PyreInfo::checkContinue(CHAR_DATA * ch, OBJ_DATA * obj, const char * materialName, unsigned int itemType, unsigned int vnum, unsigned int minLevel)
{
    // Check whether the item matches the constraints
    if (!checkMatch(obj, materialName, itemType, vnum, minLevel))
    {
        handleIncorrect(ch, obj);
        return false;
    }

    // Item checks out; spell continued
    handleCorrect(ch, obj);
    ++m_step;
    return true;
}

bool PyreInfo::checkFinishPersistent(CHAR_DATA * ch, OBJ_DATA * obj, Effect effect, const char * materialName, unsigned int itemType, unsigned int vnum, unsigned int minLevel)
{
    // Check whether the item matches the constraints
    if (!checkMatch(obj, materialName, itemType, vnum, minLevel))
    {
        handleIncorrect(ch, obj);
        return false;
    }
    
    // Assign effect and reset
    m_activePersistentSpell = effect;
    m_activePersistentSpellGreater = m_currSpellGreater;
    m_effectModifier = 0;
    reset();
    return true;
}

bool PyreInfo::checkMatch(OBJ_DATA * obj, const char * materialName, unsigned int itemType, unsigned int vnum, unsigned int minLevel)
{
    // Verify the item type
    if (itemType != Req_None && static_cast<unsigned int>(obj->item_type) != itemType)
        return false;

    // Verify the item level
    if (minLevel != Req_None && static_cast<unsigned int>(obj->level) < minLevel)
        return false;

    // Verify the vnum
    if (vnum != Req_None && static_cast<unsigned int>(obj->pIndexData->vnum) != vnum)
        return false;

    // Verify the material
    if (materialName != NULL && obj->material != material_lookup(materialName))
        return false;
    
    return true;
}

void PyreInfo::handleCorrect(CHAR_DATA * ch, OBJ_DATA * obj)
{
    act("The pyre flickers and flares as it consumes $p, and seems to grow slightly hotter!", ch, obj, NULL, TO_CHAR);
    act("The pyre flickers and flares as it consumes $p, and seems to grow slightly hotter!", ch, obj, NULL, TO_ROOM);
    check_improve(ch, NULL, gsn_bloodpyre, TRUE, 8);
}

void PyreInfo::handleIncorrect(CHAR_DATA * ch, OBJ_DATA * obj)
{
    act("The pyre flickers for a moment as it consumes $p, but nothing further happens.", ch, obj, NULL, TO_CHAR);
    act("The pyre flickers for a moment as it consumes $p, but nothing further happens.", ch, obj, NULL, TO_ROOM);
    check_improve(ch, NULL, gsn_bloodpyre, FALSE, 12);
}

void PyreInfo::rawSetColor(CHAR_DATA * ch, Color color, const char * colorName)
{
    // Build the new long description
    std::ostringstream newLong;
    newLong << "A ";
    if (colorName != NULL)
        newLong << colorName << "-tinged ";
    newLong << "pyre blazes brightly here.";

    char * newLongStr = strdup(newLong.str().c_str());
    if (newLongStr == NULL)
    {
        bug("Out of memory in handleSetColor", 0);
        send_to_char("An error has occurred; please inform the gods\n", ch);
        return;
    }

    // Set the new long description
    free_string(m_pyre->description);
    m_pyre->description = newLongStr;

    // Set the color
    m_color = color;
}

void PyreInfo::handleSetColor(CHAR_DATA * ch, OBJ_DATA * obj, Color color, const char * colorName)
{
    // Set the color and make the echoes
    rawSetColor(ch, color, colorName);
    std::ostringstream mess;
    mess << "As the pyre consumes $p, its flames take on a faintly " << colorName << " hue.";
    act(mess.str().c_str(), ch, obj, NULL, TO_CHAR);
    act(mess.str().c_str(), ch, obj, NULL, TO_ROOM);
}

bool PyreInfo::testBit(CHAR_DATA * ch, unsigned int num)
{
    return (BIT_GET(ch->pcdata->bitptr, num) != 0);
}

void PyreInfo::setBit(CHAR_DATA * ch, unsigned int num)
{
    BIT_SET(ch->pcdata->bitptr, num);
}

bool PyreInfo::canTransport(ROOM_INDEX_DATA * room)
{
    if (IS_SET(room->room_flags, ROOM_NOMAGIC) || IS_SET(room->room_flags, ROOM_SAFE)
    ||  IS_SET(room->room_flags, ROOM_PRIVATE) || IS_SET(room->room_flags, ROOM_NO_RECALL))
    return false;

    return true;
}

void PyreInfo::sendToArea(CHAR_DATA * ch, const char * message, bool excludeRoom)
{
    if (ch->in_room == NULL)
        return;

    for (DESCRIPTOR_DATA * d(descriptor_list); d != NULL; d = d->next)
    {
        if (d->connected != CON_PLAYING || d->character->in_room == NULL || d->character->in_room->area != ch->in_room->area)
            continue;

        if (excludeRoom && d->character->in_room == ch->in_room)
            continue;

        send_to_char(message, d->character);
    }
}

void PyreInfo::Proc_EssenceOfTheInferno(CHAR_DATA * ch, OBJ_DATA * obj, bool &)
{
    // Step 0: an item of type light
    // Step 1: a tempered shield
    switch (m_step)
    {
        case 0: checkContinue(ch, obj, NULL, ITEM_LIGHT, Req_None, Req_None); break;
        default:
            if (!IS_SET(obj->wear_flags, ITEM_WEAR_SHIELD))
            {
                handleIncorrect(ch, obj);
                return;
            }

            if (checkFinishPersistent(ch, obj, Effect_EssenceOfTheInferno, NULL, ITEM_ARMOR, Req_None, Req_None))
            {
                act("As $p melts into the pyre, the flames seem to part briefly, dividing down the center as though marking a fiery path.", ch, obj, NULL, TO_CHAR);
                act("The sight burns itself into your mind, and you feel the essence of the Inferno settle about you.", ch, obj, NULL, TO_CHAR);
                act("A moment later the vision is gone, and the fire burns normally once more.", ch, obj, NULL, TO_CHAR);

                act("As $p melts into the pyre, the flames seem to part briefly, dividing down the center as though marking a fiery path.", ch, obj, NULL, TO_ROOM);
                act("A moment later the vision is gone, and the fire burns normally once more.", ch, obj, NULL, TO_ROOM);
            }
            break;
    }
}

void PyreInfo::Proc_WreathOfFlames(CHAR_DATA * ch, OBJ_DATA * obj, bool &)
{
    // Step 0: an item made of gold
    // Step 1: a metal necklace
    switch (m_step)
    {
        case 0: checkContinue(ch, obj, "gold", Req_None, Req_None, Req_None); break;
        default: 
            if (!IS_SET(obj->wear_flags, ITEM_WEAR_NECK) || !material_table[obj->material].metal)
            {
                handleIncorrect(ch, obj);
                return;
            }

            if (checkFinishPersistent(ch, obj, Effect_WreathOfFlame, NULL, Req_None, Req_None, Req_None))
            {
                act("As $p melts into the pyre, a shimmering sphere of burning light seems to form about you.", ch, obj, NULL, TO_CHAR);
                act("With senses not wholly your own, you feel the sphere collapse inward, fading from sight as it forms a fiery core within you.", ch, obj, NULL, TO_CHAR);
                act("As $p melts into the pyre, a shimmering sphere of burning light seems to settle about $n for a moment before slowly fading away.", ch, obj, NULL, TO_ROOM);
            }
            break;
    }
}

void PyreInfo::Proc_FuryOfAdaChemtaBoghor(CHAR_DATA * ch, OBJ_DATA * obj, bool &)
{
    // Step 0: a ruby
    if (checkFinishPersistent(ch, obj, Effect_FuryOfAdaChemtaBoghor, "ruby", ITEM_GEM, Req_None, Req_None))
    {
        act("As you drop $p into the pyre, the flames begin to sputter and flare angrily! They swell up, threatening to consume the area!", ch, obj, NULL, TO_CHAR);
        act("As $n drops $p into the pyre, the flames begin to sputter and flare angrily! They swell up, threatening to consume the area!", ch, obj, NULL, TO_ROOM);

        // Apply a blazing inferno to the room
        AFFECT_DATA af  = {0};
        af.where        = TO_ROOM_AFF;
        af.type         = gsn_blazinginferno;
        af.level        = ch->level;
        af.duration     = 12;
        affect_to_room(ch->in_room, &af);

        // Possibly apply a rain of fire to the area
        if (isEffectGreater() && !area_is_affected(ch->in_room->area, gsn_rainoffire))
        {
            act("Even the air does not escape unscathed, instead darkening into a reddish haze which burns with every breath.", ch, obj, NULL, TO_CHAR);
            act("Even the air does not escape unscathed, instead darkening into a reddish haze which burns with every breath.", ch, obj, NULL, TO_ROOM);
            sendToArea(ch, "The air seems to darken into a reddish haze, and each breath begins to burn painfully.\n", true);

            AFFECT_DATA raf = {0};
            raf.where       = TO_AREA;
            raf.type        = gsn_rainoffire;
            raf.level       = ch->level;
            raf.duration    = 12;
            affect_to_area(ch->in_room->area, &raf);
        }
    }
}

void PyreInfo::Proc_BrimstoneConduit(CHAR_DATA * ch, OBJ_DATA * obj, bool &)
{
    // Step 0: an item made of sulphur
    if (checkFinishPersistent(ch, obj, Effect_BrimstoneConduit, "sulphur", Req_None, Req_None, Req_None))
    {
        std::ostringstream mess;
        mess << "Even as $p fills the air with the foul reek of burning brimstone, you sense a ";
        mess << (isEffectGreater() ? "very powerful " : "");
        mess << "link to the core of the Inferno open up within your mind.";
        act(mess.str().c_str(), ch, obj, NULL, TO_CHAR);
        act("As $p burns away, the air is filled with the foul reek of burning brimstone.", ch, obj, NULL, TO_ROOM);
    }
}

void PyreInfo::Proc_BayyalsRealm(CHAR_DATA * ch, OBJ_DATA * obj, bool &)
{
    // Step 0: a wand
    // Step 1: a tooth from a dragon
    switch (m_step)
    {
        case 0: checkContinue(ch, obj, NULL, ITEM_WAND, Req_None, Req_None); break;
        default: 
            MOB_INDEX_DATA * mobIndex(get_mob_index(obj->value[0]));
            if (mobIndex == NULL || mobIndex->race != race_lookup("dragon"))
            {
                handleIncorrect(ch, obj);
                return;
            }

            if (checkFinishPersistent(ch, obj, Effect_BayyalsRealm, NULL, Req_None, OBJ_VNUM_TROPHY_TOOTH, Req_None))
            {
                act("You are suddenly seized with a vision of boundless, burning hatred, and a malevolent power far beyond that of your own momentarily grips you!", ch, obj, NULL, TO_CHAR);
                do_yell(ch, const_cast<char*>("Akbay mu iml e lliza Bayyali!"));
                act("The pyre leaps high, roaring with newfound power as a potent wave of heat sweeps across the area!", ch, obj, NULL, TO_CHAR);
                act("The pyre leaps high, roaring with newfound power as a potent wave of heat sweeps across the area!", ch, obj, NULL, TO_ROOM);
                sendToArea(ch, "A potent wave of heat sweeps across the area!\n", true);
            }
            break; 
    }
}

void PyreInfo::Proc_Heatlash(CHAR_DATA * ch, OBJ_DATA * obj, bool &)
{
    // Step 0: a whip
    // Step 1: a gold coin
    // Step 2: an item made of stone or smoke
    switch (m_step)
    {
        case 0: 
            if (obj->value[0] != WEAPON_WHIP)
            {
                handleIncorrect(ch, obj);
                return;
            }

            checkContinue(ch, obj, NULL, ITEM_WEAPON, Req_None, Req_None);
            break;

        case 1:
            if (obj->value[0] != 1 || obj->value[1] != C_GOLD)
            {
                handleIncorrect(ch, obj);
                return;
            }

            checkContinue(ch, obj, NULL, ITEM_MONEY, Req_None, Req_None);
            break;

        default:
            unsigned int modifier;
            if (obj->material == material_lookup("stone")) modifier = Heatlash_Modifier_Earth;
            else if (obj->material == material_lookup("smoke")) modifier = Heatlash_Modifier_Fire;
            else
            {
                handleIncorrect(ch, obj);
                return;
            }

            if (checkFinishPersistent(ch, obj, Effect_Heatlash, NULL, Req_None, Req_None, Req_None))
            {
                setEffectModifier(modifier);
                act("As $p is unmade in the pyre, a tongue of flame lashes out, enveloping you!", ch, obj, NULL, TO_CHAR);
                act("As $p is unmade in the pyre, a tongue of flame lashes out, momentarily enveloping $n!", ch, obj, NULL, TO_ROOM);
                send_to_char("The heat quickly recedes, leaving you feeling hardened.\n", ch);
            }
            break;
    }
}

void PyreInfo::Proc_HungerOfTheFlame(CHAR_DATA * ch, OBJ_DATA * obj, bool &)
{
    // Step 0: a container of red wine
    // Step 1: a PC heart
    switch (m_step)
    {
        case 0:
            if (obj->value[2] != liq_lookup("red wine"))
            {
                handleIncorrect(ch, obj);
                return;
            }

            checkContinue(ch, obj, NULL, ITEM_DRINK_CON, Req_None, Req_None);
            break;

        default:
            if (obj->value[0] > 0 || (obj->pIndexData->vnum != OBJ_VNUM_TROPHY_HEART && obj->pIndexData->vnum != OBJ_VNUM_TROPHY_BRAINS && obj->pIndexData->vnum != OBJ_VNUM_TROPHY_TONGUE))
            {
                handleIncorrect(ch, obj);
                return;
            }
            
            act("The flames seem to stretch towards $p as though eager to consume it!", ch, obj, NULL, TO_CHAR);
            act("The flames seem to stretch towards $p as though eager to consume it!", ch, obj, NULL, TO_ROOM);

            // Check for level
            const char * mess(NULL);
            if (obj->level >= 30)
            {
                switch (obj->pIndexData->vnum)
                {
                    // Hearts give HP bonuses
                    case OBJ_VNUM_TROPHY_HEART:
                        if (!testBit(ch, Bit_NoMoreHP)) 
                        {
                            if (number_percent() < 25)
                                setBit(ch, Bit_NoMoreHP);

                            ch->max_hit += dice(3, 5);
                            mess = "You feel your hardiness grow, fueled by your link to the Inferno!\n";
                        }
                        break;

                    case OBJ_VNUM_TROPHY_BRAINS:
                        if (!testBit(ch, Bit_NoMoreMana))
                        {
                            if (number_percent() < 25)
                                setBit(ch, Bit_NoMoreMana);

                            ch->max_mana += dice(3, 10);
                            mess = "You sense the power of your mind expand, fueled by your link to the Inferno!\n";
                        }
                        break;

                    case OBJ_VNUM_TROPHY_TONGUE:
                        if (obj->value[2] >= 0)
                        {
                            int & langSkill(ch->pcdata->learned[*lang_data[race_table[obj->value[2]].native_tongue].sn]);
                            if (langSkill < 100)
                            {
                                langSkill += dice(3, 10);
                                langSkill = UMIN(100, langSkill);
                                mess = "Something clicks in your mind, and your linguistic capabilities improve!\n";
                            }
                        }
                        break;
                }
            }

            std::ostringstream logmess;
            logmess << ch->name << " just offered " << obj->short_descr << " as a burnt offering ";

            // Perform echoes
            if (mess == NULL)
            {
                logmess << " but receives no bonus";
                act("$p vanishes swiftly in the fire, but the flames seem diminshed somehow, almost as if...disappointed.", ch, obj, NULL, TO_CHAR);
                act("$p vanishes swiftly in the fire, but the flames seem diminshed somehow, almost as if...disappointed.", ch, obj, NULL, TO_ROOM);
            }
            else
            {
                logmess << " and receives a bonus";
                act("$p vanishes swiftly in the greedy flames, which flare brightly and hiss as if in appreciation.", ch, obj, NULL, TO_CHAR);
                act("$p vanishes swiftly in the greedy flames, which flare brightly and hiss as if in appreciation.", ch, obj, NULL, TO_ROOM);
                send_to_char(mess, ch);
            }

            wiznet(const_cast<char*>(logmess.str().c_str()), NULL, NULL, WIZ_ACTIVITY, 0, 0);
            log_string(logmess.str().c_str());
            reset();
            break;
    }
}

void PyreInfo::Proc_Fireflies(CHAR_DATA * ch, OBJ_DATA * obj, bool &)
{
    // Step 0: 144 silver coins
    if (obj->value[0] != 144 || obj->value[1] != C_SILVER || obj->item_type != ITEM_MONEY)
    {
        handleIncorrect(ch, obj);
        return;
    }

    // Do echoes
    act("As the coins melt away in the pyre, the flames to freeze for a moment.", ch, obj, NULL, TO_CHAR);
    act("A low buzzing noise seems to emanante from within the pyre's depth, soft at first but growing steadily louder.", ch, obj, NULL, TO_CHAR);
    act("Suddenly a swarm of tiny fire imps bursts out from the flames, held aloft by miniscule wings of flames!", ch, obj, NULL, TO_CHAR);
    act("The swarm of imps, chittering madly, quickly speeds off! Soon there are thousands of little points of light all over the area!", ch, obj, NULL, TO_CHAR);

    act("As the coins melt away in the pyre, the flames to freeze for a moment.", ch, obj, NULL, TO_ROOM);
    act("A low buzzing noise seems to emanante from within the pyre's depth, soft at first but growing steadily louder.", ch, obj, NULL, TO_ROOM);
    act("Suddenly a swarm of tiny fire imps bursts out from the flames, held aloft by miniscule wings of flames!", ch, obj, NULL, TO_ROOM);
    act("The swarm of imps, chittering madly, quickly speeds off! Soon there are thousands of little points of light all over the area!", ch, obj, NULL, TO_ROOM);

    sendToArea(ch, "A swarm of tiny glowing fire imps suddenly flies in on wings of flames, moving erratically and gibbering crazily!\n", true);

    // Time for the big reveal
    for (DESCRIPTOR_DATA * d = descriptor_list; d->next != NULL; d = d->next)
    {
        // Filter out certain characters
        if (d->connected != CON_PLAYING || d->character->in_room == NULL 
        ||  d->character->in_room->area != ch->in_room->area 
        ||  IS_IMMORTAL(d->character) || is_affected(d->character, gsn_reveal))
            continue;

        if (is_safe_spell(ch, d->character, TRUE) || saves_spell(ch->level, ch, d->character, DAM_FIRE))
        {
            send_to_char("The fire imps try to latch onto you, but you resist their attempts!\n", d->character);
            continue;
        }

        // Add the affect
        AFFECT_DATA af = {0};
        af.where     = TO_AFFECTS;
        af.type      = gsn_reveal;
        af.level     = ch->level;
        af.duration  = 6 + ((get_skill(ch, gsn_bloodpyre) - 60) / 4);
        af.modifier  = 1;
        af.bitvector = AFF_FAERIE_FIRE;
        affect_to_char(d->character, &af);
        
        act("You try to swat the imps away, but there are too many! They flitter all about you, cackling gleefully!", d->character, NULL, NULL, TO_CHAR);
        act("The imps flitter all about $n, cackling with glee!", d->character, NULL, NULL, TO_ROOM);
        
        // No more hiding
        uncamo_char(d->character);
        unhide_char(d->character);
        affect_strip(d->character, gsn_invis);
        affect_strip(d->character, gsn_mass_invis);
        affect_strip(d->character, gsn_wildmove);
        affect_strip(d->character, gsn_sneak);

        // Strip bits unless inherent
        if (!(race_table[d->character->race].aff & AFF_SNEAK))
            REMOVE_BIT(d->character->affected_by, AFF_SNEAK);

        if (!(race_table[d->character->race].aff & AFF_HIDE))
            REMOVE_BIT(d->character->affected_by, AFF_HIDE);

        if (!(race_table[d->character->race].aff & AFF_INVISIBLE))
            REMOVE_BIT(d->character->affected_by, AFF_INVISIBLE);
    }

    reset();
}

void PyreInfo::Proc_InfernosReach(CHAR_DATA * ch, OBJ_DATA * obj, bool &)
{
    // Step 0: a wand
    // Step 1: an arrow
    // Step 2: a item once held by the target
    // Step 3: an object of type writing with the target's name on it
    switch (m_step)
    {
        case 0: checkContinue(ch, obj, NULL, ITEM_WAND, Req_None, Req_None); break;
        case 1: checkContinue(ch, obj, NULL, ITEM_ARROW, Req_None, Req_None); break;
        case 2:
            // Save off the last owners
            for (unsigned int i(0); i < MAX_LASTOWNER; ++i)
            {
                if (obj->lastowner[i][0] == '\0')
                    break;

                m_lastOwners.push_back(obj->lastowner[i]);
            }
            checkContinue(ch, obj, NULL, Req_None, Req_None, Req_None);
            break;

       default:
            if (obj->item_type != ITEM_WRITING)
            {
                handleIncorrect(ch, obj);
                return;
            }

            // Look for one of the owners in the text
            std::string * targetName(NULL);
            for (EXTRA_DESCR_DATA * ed(obj->extra_descr); targetName == NULL && ed != NULL; ed = ed->next)
            {
                for (size_t i(0); i < m_lastOwners.size(); ++i)
                {
                    if (!str_infix(m_lastOwners[i].c_str(), ed->description))
                    {
                        targetName = &m_lastOwners[i];
                        break;
                    }
                }
            }

            // Did not find any available target name
            if (targetName == NULL)
            {
                handleIncorrect(ch, obj);
                return;
            }

            // Locate the target
            CHAR_DATA * victim(get_char_world(ch, const_cast<char*>(targetName->c_str())));
            if (victim == NULL)
            {
                handleIncorrect(ch, obj);
                return;
            }

            act("As $p burns away, the pyre leaps brightly into the sky!", ch, obj, NULL, TO_CHAR);
            act("You sense a thin tendril of fiery power lance towards the horizon, seeking $N.", ch, obj, victim, TO_CHAR);
            act("As $p burns away, the pyre leaps brightly into the sky!", ch, obj, NULL, TO_ROOM);
            
            // Check for a save
            if (is_safe_spell(ch, victim, FALSE) || savesConsumeSpecial(ch, victim) || saves_spell(ch->level + 10, ch, victim, DAM_FIRE) || (victim->in_room != NULL && is_symbol_present(*victim->in_room, OBJ_VNUM_SYMBOL_PROTECT)))
            {
                send_to_char("You feel a wave of heat strike you suddenly, but it soon passes.\n", victim);
                act("You sense the questing energy strike $M, but to no effect.", ch, obj, victim, TO_CHAR);
            }
            else
            {
                send_to_char("A wave of heat slams suddenly into you, and you cry out as your skin begins to burn painfully!\n", victim);
                act("$n cries out in sudden pain as $s skin begins to glow hot!", victim, NULL, NULL, TO_ROOM);
                act("You sense the questing energy find $M, striking deep at $S core!", ch, obj, victim, TO_CHAR);

                // Apply the effect
                AFFECT_DATA af = {0};
                af.where     = TO_AFFECTS;
                af.type      = gsn_consume;
                af.level     = ch->level;
                af.duration  = 6 + ((get_skill(ch, gsn_bloodpyre) - 50) / 5);
                affect_to_char(victim, &af);
            }

            reset();
            break;
    }
}

void PyreInfo::Proc_SummonSalamander(CHAR_DATA * ch, OBJ_DATA * obj, bool &)
{
    // Step 0: 50 gold coins
    if (obj->value[0] < 33 || obj->value[1] != C_GOLD || obj->item_type != ITEM_MONEY)
    {
        handleIncorrect(ch, obj);
        return;
    }

    act("As the coins start to melt together, there is a skittering sound from within the pyre.", ch, obj, NULL, TO_CHAR);
    act("As the coins start to melt together, there is a skittering sound from within the pyre.", ch, obj, NULL, TO_ROOM);
    act("Suddenly a serpentine head appears in the flames! With a single quick motion it snatches up the gold and gulps it down.", ch, obj, NULL, TO_CHAR);
    act("Suddenly a serpentine head appears in the flames! With a single quick motion it snatches up the gold and gulps it down.", ch, obj, NULL, TO_ROOM);

    if (ch->pet != NULL)
    {
        act("The reptilian eyes turn their gaze on $N, and with a hissing sound, the creature vanishes in a puff of smoke!", ch, obj, ch->pet, TO_CHAR);
        act("The reptilian eyes turn their gaze on $N, and with a hissing sound, the creature vanishes in a puff of smoke!", ch, obj, ch->pet, TO_ROOM);
    }
    else
    {
        // Build the salamander
        int modifier = m_currSpellGreater ? 4 : 3;
        CHAR_DATA * pet(create_mobile(get_mob_index(MOB_VNUM_SALAMANDER)));
        pet->level = ch->level;
        pet->damroll = (((ch->level / 3) + ((get_skill(ch, gsn_bloodpyre) - 70) / 5)) * modifier) / 3;
        pet->hitroll = pet->damroll;
        pet->damage[0] = 2;
        pet->damage[1] = (((ch->level / 3) + UMAX(0, (get_skill(ch, gsn_bloodpyre) - 70) / 2)) * modifier) / 3;
        pet->damage[2] = (ch->level / 10);
        pet->hit = ((dice((ch->level / 3) + UMAX(0, get_skill(ch, gsn_bloodpyre) - 75), 3) * 10 + (dice(2, 50))) * modifier) / 3;
        pet->max_hit = pet->hit;
        char_to_room(pet, ch->in_room);
        ch->pet = pet;
        pet->master = ch;
        pet->leader = ch;

        act("With a hissing sound, a lizardlike form bounds from the pyre, smoke streaming from its nostrils!", ch, obj, NULL, TO_CHAR);
        act("With a hissing sound, a lizardlike form bounds from the pyre, smoke streaming from its nostrils!", ch, obj, NULL, TO_ROOM);
        act("On large, clawed feet, $N patters into position behind you.", ch, obj, pet, TO_CHAR);
        act("On large, clawed feet, $N patters into position behind $n.", ch, obj, pet, TO_ROOM);
    }

    reset();
}

void PyreInfo::Proc_TrailOfEmbers(CHAR_DATA * ch, OBJ_DATA * obj, bool &)
{
    // Step 0: an item worn on the feet
    // Step 1: copper coins equal in number to the target campfire
    switch (m_step)
    {
        case 0: 
            if (!IS_SET(obj->wear_flags, ITEM_WEAR_FEET))
            {
                handleIncorrect(ch, obj);
                return;
            }

            checkContinue(ch, obj, NULL, Req_None, Req_None, Req_None);
            break;

        default:
            if (obj->value[1] != C_COPPER || obj->item_type != ITEM_MONEY)
            {
                handleIncorrect(ch, obj);
                return;
            }

            // Check for a cursed room
            if (!canTransport(ch->in_room))
            {
                act("The pyre begins to flare up in dazzling colors of orange and white, but then fizzles, brought low by the powers of this place.", ch, obj, NULL, TO_CHAR);
                act("The pyre begins to flare up in dazzling colors of orange and white, but then fizzles.", ch, obj, NULL, TO_ROOM);
            }
            else
            {
                // Check for a good target fire
                unsigned int targetFire(static_cast<unsigned int>(obj->value[0] - 1));
                std::vector<OBJ_DATA *> fires(lookupGroundFires(ch, lookupGroundFireHere(ch), false));
                if (static_cast<unsigned int>(targetFire) >= fires.size() || fires[targetFire]->in_room == NULL)
                {
                    act("The pyre begins to flare up in dazzling colors of orange and white, but then fizzles when it cannot form a link through the Inferno.", ch, obj, NULL, TO_CHAR);
                    act("The pyre begins to flare up in dazzling colors of orange and white, but then fizzles.", ch, obj, NULL, TO_ROOM);
                }
                else
                {
                    // Transport the caster
                    act("The pyre flares up in dazzling colors of orange and white, forming a blazing archway!", ch, obj, NULL, TO_CHAR);
                    act("You step through, and pain explodes across your body! Time seems to stop for a moment as your essence draws dangerously near the Inferno itself!", ch, obj, NULL, TO_CHAR);
                    act("Suddenly the pain ceases as you step back into the world, your being more or less intact.", ch, obj, NULL, TO_CHAR);

                    act("The pyre flares up in dazzling colors of orange and white, forming a blazing archway!", ch, obj, NULL, TO_ROOM);
                    act("$n steps through the burning portal, and $s body seems to burst into flame before suddenly vanishing!", ch, obj, NULL, TO_ROOM);

                    char_from_room(ch);
                    char_to_room(ch, fires[targetFire]->in_room);
                    do_look(ch, const_cast<char*>("auto"));

                    act("$n suddenly appears in a burst of heat and orange-white light!", ch, NULL, NULL, TO_ROOM);
                    damage_new(ch, ch, dice(3, 100), TYPE_HIT, DAM_FIRE, true, const_cast<char*>("burning passage"));
                }
            }

            reset();
            break;
    }
}

void PyreInfo::Proc_Steelscald(CHAR_DATA * ch, OBJ_DATA * obj, bool &)
{
    // Step 0: a weapon made of steel
    // Step 1: a container of water
    switch (m_step)
    {
        case 0: checkContinue(ch, obj, "steel", ITEM_WEAPON, Req_None, Req_None); break;
        default:
            if (obj->item_type != ITEM_DRINK_CON || obj->value[1] <= 0 ||  obj->value[2] != liq_lookup("water"))
            {
                handleIncorrect(ch, obj);
                return;
            }

            act("The pyre crackles and pops as $p is added to it, then begins to flare up!", ch, obj, NULL, TO_CHAR);
            act("The pyre crackles and pops as $p is added to it, then begins to flare up!", ch, obj, NULL, TO_ROOM);

            // Apply heat metal to all characters in the area
            for (DESCRIPTOR_DATA * d = descriptor_list; d->next != NULL; d = d->next)
            {
                // Filter out certain characters
                if (d->connected != CON_PLAYING || d->character->in_room == NULL
                ||  d->character->in_room->area != ch->in_room->area || d->character == ch)
                        continue;

                send_to_char("The temperature around you abruptly increases!\n", d->character);
                if (is_safe_spell(ch, d->character, FALSE))
                    continue;

                // Hit them for their metal objects
                bool first(true);
                for (OBJ_DATA * gear(d->character->carrying); gear != NULL; gear = gear->next_content)
                {
                    if (gear->worn_on && material_table[gear->material].metal)
                    {
                        if (first)
                        {
                            send_to_char("All your metal gear grows alarming hot!\n", d->character);
                            first = false;
                        }

                        act("$p burns $n!", d->character, gear, NULL, TO_ROOM);
                        act("$p burns you!", d->character, gear, NULL, TO_CHAR);

                        int damage = dice(2, ch->level / 7) + (get_skill(ch, gsn_bloodpyre) - 75) / 5;
                        if (saves_spell(ch->level, ch, d->character, DAM_FIRE))
                            damage /= 2;

                        damage_new(d->character, d->character, damage, gsn_heat_metal, DAM_FIRE, false, const_cast<char*>(""));
                    }
                }
            }
            reset();
            break;
    }
}

void PyreInfo::Proc_FlickerFyre(CHAR_DATA * ch, OBJ_DATA * obj, bool &)
{
    // Step 0: an item of material smoke
    // Step 1: an item of type light
    // Step 2: an item of type key
    // Step 3: an item of type net
    // Step 4: an item of type writing with the target's name written on it
    switch (m_step)
    {
        case 0: checkContinue(ch, obj, "smoke", Req_None, Req_None, Req_None); break;
        case 1: checkContinue(ch, obj, NULL, ITEM_LIGHT, Req_None, Req_None); break;
        case 2: checkContinue(ch, obj, NULL, ITEM_KEY, Req_None, Req_None); break;
        case 3: checkContinue(ch, obj, NULL, ITEM_NET, Req_None, Req_None); break;
        default:
            if (obj->item_type != ITEM_WRITING)
            {
                handleIncorrect(ch, obj);
                return;
            }

            // Try to find a target with the written name
            CHAR_DATA * victim(NULL);
            for (EXTRA_DESCR_DATA * ed(obj->extra_descr); victim == NULL && ed != NULL; ed = ed->next)
                victim = get_char_world(ch, ed->description);

            if (victim == NULL)
            {
                handleIncorrect(ch, obj);
                return;
            }

            // Log this
            std::ostringstream logmess;
            logmess << ch->name << " just used Flickerfyre to view " << victim->name << "'s past";
            wiznet(const_cast<char*>(logmess.str().c_str()), NULL, NULL, WIZ_ACTIVITY, 0, 0);
            log_string(logmess.str().c_str());

            // Handle the effect result
            act("As $p burns away, the fires of $n's pyre begin to sputter and smoke.", ch, obj, NULL, TO_ROOM);
            if (victim->pcdata == NULL || victim->pcdata->background == NULL || victim->pcdata->background[0] == '\0')
                act("As $p burns away, the fires of your pyre begin to sputter and smoke...but you can make out nothing in the haze.", ch, obj, NULL, TO_CHAR);
            else
            {
                // Build the random seed based on the target's name
                unsigned int seed = 12345;
                for (unsigned int i(0); victim->name[i] != '\0'; ++i)
                    seed += (static_cast<unsigned int>(victim->name[i]) * (i + 1));
                srand(seed);

                // Break the background down into words
                std::vector<std::string> words;
                std::istringstream wordParser(victim->pcdata->background);
                while (!wordParser.eof())
                {
                    std::string word;
                    wordParser >> word;
                    if (!word.empty())
                        words.push_back(word);
                }

                // Select phrases at random
                std::vector<std::string> phrases;
                bool revealing(false);
                unsigned int wordsSinceChange(0);
                for (unsigned int i(0); i < words.size(); ++i)
                {
                    ++wordsSinceChange;

                    if (revealing)
                    {
                        // Check whether to stop revealing
                        if (rand() % 100 < (int)(wordsSinceChange * (m_currSpellGreater ? 2 : 4)))
                        {
                            revealing = false;
                            wordsSinceChange = 0;
                            continue;
                        }

                        // Still revealing
                        phrases[phrases.size() - 1] += ' ';
                        phrases[phrases.size() - 1] += words[i];
                        continue;
                    }
                    
                    // Check whether to start revealing
                    if (rand() % 100 < (int)(wordsSinceChange / (m_currSpellGreater ? 1 : 2)))
                    {
                        revealing = true;
                        wordsSinceChange = 0;
                        phrases.push_back(words[i]);
                    }
                }

                struct CallbackHandler
                {
                    static bool HandleMoveFrom(CHAR_DATA * ch, ROOM_INDEX_DATA * room, EchoAffect * thisAffect, void * tag)
                    {
                        send_to_char("You tear your gaze away from the hazy images, and realize you are burning up with fever!\n", ch);
                        FinishSpell(ch, thisAffect, tag);
                        return true;
                    }

                    static bool HandlePositionChange(CHAR_DATA * ch, int newPos, EchoAffect * thisAffect, void * tag)
                    {
                        switch (newPos)
                        {
                            case POS_SITTING:
                            case POS_RESTING:
                            case POS_STANDING:
                                return false;
                        }

                        send_to_char("You tear your gaze away from the hazy images, and realize you are burning up with fever!\n", ch);
                        FinishSpell(ch, thisAffect, tag);
                        return true;
                    }

                    static bool FinishSpell(CHAR_DATA * ch, EchoAffect *, void * tag)
                    {
                        AFFECT_DATA af = {0};
                        af.type = gsn_fever;
                        af.level = ch->level;
                        af.bitvector = AFF_PLAGUE;
                        af.modifier = -150;
                        af.location = APPLY_MANA;
                        af.duration = 6;
                        af.where = TO_AFFECTS;
                        affect_to_char(ch, &af);
                        return false;
                    }
                };

                // Handle no phrases found
                if (phrases.empty()) 
                    act("As $p burns away, the fires of your pyre begin to sputter and smoke...but you can make out nothing in the haze.", ch, obj, NULL, TO_CHAR);
                else
                {
                    // Success, build the echoes
                    act("As $p burns away, the fires of your pyre begin to sputter and smoke.", ch, obj, NULL, TO_CHAR);
                    
                    EchoAffect * echoAff(new EchoAffect(1));
                    echoAff->SetMoveCallback(&CallbackHandler::HandleMoveFrom);
                    echoAff->SetPositionCallback(&CallbackHandler::HandlePositionChange);
                    
                    if (ch->race == global_int_race_shuddeni)
                        echoAff->AddLine(NULL, "Hazy images begin to take shape before you, briefly formed by the curling wisps before vanishing again into amorphous obscurity.");
                    else
                        echoAff->AddLine(NULL, "Hazy images begin to take shape before your eyes, briefly formed by the curling wisps before vanishing again into amorphous obscurity.");
                    
                    // No more than 10 - 15 phrases from any background
                    unsigned int maxCount(m_currSpellGreater ? 15 : 10);
                    for (unsigned int i(0); i < maxCount && !phrases.empty(); ++i)
                    {
                        // Choose a random phrase
                        unsigned int index(0);
                        if (phrases.size() > 1)
                            index = static_cast<unsigned int>(rand() % phrases.size());
                        
                        // Echo the random phrase and strike it from the list
                        echoAff->AddLine(NULL, "..." + phrases[index] + "...");
                        phrases.erase(phrases.begin() + index);
                    }

                    echoAff->AddLine(&CallbackHandler::FinishSpell, "The last of the images drifts away, and you realize your head is pounding with fever!"); 
                    EchoAffect::ApplyToChar(ch, echoAff);
                }
            }

            reset();
            break;
    }
}

void PyreInfo::Proc_Heatmine(CHAR_DATA * ch, OBJ_DATA * obj, bool &)
{
    // Step 0: an object of material glass
    if (obj->material != material_lookup("glass"))
    {
        handleIncorrect(ch, obj);
        return;
    }

    if (room_is_affected(ch->in_room, gsn_heatmine))
    {
        act("$p shatters in the heat, and the ward here flares slightly.", ch, obj, NULL, TO_CHAR);
        act("$p shatters in the heat, and the brimstone ring flares slighly.", ch, obj, NULL, TO_ROOM);
    }
    else if (is_affected(ch, gsn_heatmine))
    {
        act("$p shatters in the heat, but you sense you aren't yet ready to call forth another brimstone ward.", ch, obj, NULL, TO_CHAR);
        act("$p shatters in the heat.", ch, obj, NULL, TO_ROOM);
    }
    else
    {
        act("$p shatters in the heat, and a ring of smoldering brimstone suddenly forms near the pyre!", ch, obj, NULL, TO_CHAR);
        act("$p shatters in the heat, and a ring of smoldering brimstone suddenly forms near the pyre!", ch, obj, NULL, TO_ROOM);

        // Add the effect
        AFFECT_DATA af = {0};
        af.where     = TO_ROOM;
        af.type      = gsn_heatmine;
        af.level     = ch->level;
        af.duration  = 7 + (ch->level / 10);
        af.modifier  = (m_currSpellGreater ? 1 : 0);
        affect_to_room(ch->in_room, &af);

        // Add a cooldown
        af.where     = TO_AFFECTS;
        af.duration  = 10 - ((get_skill(ch, gsn_bloodpyre) - 75) / 5);
        affect_to_char(ch, &af);
    }

    reset();
}

void PyreInfo::Proc_Hearthglow(CHAR_DATA * ch, OBJ_DATA * obj, bool & destroyObject)
{
    // Step 0: an item of material fur
    // Step 1: a single gold coin
    // Step 2: any object at all
    switch (m_step)
    {
        case 0: checkContinue(ch, obj, "fur", Req_None, Req_None, Req_None); break;
        case 1: 
            if (obj->value[0] != 1 || obj->value[1] != C_GOLD)
            {
                handleIncorrect(ch, obj);
                return;
            }

            checkContinue(ch, obj, NULL, ITEM_MONEY, Req_None, Req_None); 
            break;
        
        default:
            act("$p lands in the pyre, but does not burn! Instead, it begins to take on a soft, warm glow.", ch, obj, NULL, TO_CHAR);
            act("$p lands in the pyre, but does not burn! Instead, it begins to take on a soft, warm glow.", ch, obj, NULL, TO_ROOM);
            act("After a moment, you reach in and fish it out, knowing instinctively that just now the flames will do you no harm.", ch, obj, NULL, TO_CHAR);
            act("After a moment, $n reaches in and fishes it out, remaining somehow unharmed.", ch, obj, NULL, TO_ROOM);
            destroyObject = false;
            SET_BIT(obj->extra_flags[0], ITEM_GLOW);
            SET_BIT(obj->extra_flags[0], ITEM_WARM);
            SET_BIT(obj->extra_flags[0], ITEM_BURN_PROOF);
            reset();
            break;
    }
}

void PyreInfo::Proc_Flamekiss(CHAR_DATA * ch, OBJ_DATA * obj, bool &)
{
    // Step 0: a single silver coin
    if (obj->value[0] != 1 || obj->value[1] != C_SILVER || obj->item_type != ITEM_MONEY)
    {
        handleIncorrect(ch, obj);
        return;
    }

    act("The pyre flickers briefly.", ch, obj, NULL, TO_ROOM);
    if (is_affected(ch, gsn_burnout))
    {
        act("You feel a surge of heat flow from the pyre to you, but you are not yet ready to sustain it!", ch, obj, NULL, TO_CHAR);
    }
    else
    {
        act("You feel a surge of heat flow from the pyre to you, and your body surges with energy!", ch, obj, NULL, TO_CHAR);
        
        // Add the effect
        AFFECT_DATA af = {0};
        af.where     = TO_OAFFECTS;
        af.type      = gsn_burnout;
        af.level     = ch->level;
        af.duration  = (ch->level * 4) / 15;
        af.location  = APPLY_HIT;
        af.modifier  = 25 + (get_skill(ch, gsn_bloodpyre) - 75);
        af.bitvector = AFF_BURNOUT;
        affect_to_char(ch, &af);
    }

    reset();
}

void PyreInfo::Proc_Flamecall(CHAR_DATA * ch, OBJ_DATA * obj, bool &)
{
    // Step 0: an object of type writing with the target's name on it
    if (obj->item_type != ITEM_WRITING)
    {
        handleIncorrect(ch, obj);
        return;
    }

    // Try to find a target with the written name
    CHAR_DATA * victim(NULL);
    for (EXTRA_DESCR_DATA * ed(obj->extra_descr); victim == NULL && ed != NULL; ed = ed->next)
        victim = get_char_world(ch, ed->description);

    if (victim == NULL || is_safe_spell(ch, victim, FALSE) || victim->in_room == NULL)
    {
        handleIncorrect(ch, obj);
        return;
    }

    act("The flames flicker and dance about as you add $p, and you get a sense of questing energy branching out from them.", ch, obj, NULL, TO_CHAR);
    act("The flames flicker and dance about as $n adds $p.", ch, obj, NULL, TO_ROOM);

    // Make sure this person can be targeted; requires a ground fire or certain fire affects
    if (lookupGroundFireHere(victim) == NULL && !is_affected(victim, gsn_aspectoftheinferno) && !is_affected(victim, gsn_heartfire))
        act("You sense that $N's connection to the Inferno is too weak right now for you to call $M through the flames.", ch, obj, victim, TO_CHAR);
    else if (!canTransport(victim->in_room) || !canTransport(ch->in_room)
         || (!IS_NPC(victim) && IS_SET(victim->act, PLR_NOSUMMON) && saves_spell(ch->level, ch, victim, DAM_FIRE))
         ||  (IS_NPC(victim) && IS_SET(victim->act, ACT_NOSUBDUE)))
        act("You sense the questing power fizzle out as it approaches $N.", ch, obj, victim, TO_CHAR);
    else if (victim->in_room == ch->in_room)
        act("There is no need to summon $N when $E is already here.", ch, obj, victim, TO_CHAR);
    else
    {
        // Success; bring the target here
        act("A flicker of orange-white light coalesces momentarily about $n, then $e suddenly vanishes!", victim, NULL, NULL, TO_ROOM);
        act("A flicker of orange-white light suffuses your vision, then the world spins dizzily!", victim, NULL, NULL, TO_CHAR);
        act("The next thing you know, you are in a new location!", victim, NULL, NULL, TO_CHAR);

        act("The questing energy finds its target, and with a burst of orange-white light, $N suddenly appears!", ch, obj, victim, TO_CHAR);
        act("With a burst of orange-white light, $N suddenly appears!", ch, obj, victim, TO_ROOM);

        char_from_room(victim);
        char_to_room(victim, ch->in_room);
        do_look(victim, const_cast<char*>("auto"));

        // NPCs might attack the summoner
        if (IS_NPC(victim))
        {
            // Target NPCs 20 or more levels below the caster will just cower
            if (victim->level + 20 <= ch->level)
            {
                act("$N cowers away from you!", ch, obj, victim, TO_CHAR);
                act("$N cowers away from $n!", ch, obj, victim, TO_ROOM);
            }
            else
            {
                // If you summon a good mob, it will attack you if you are evil but not otherwise (benefit of the doubt)
                // If you summon a neutral mob, it will attack you with 50% odds unless you are good, in which case it will attack you with 25% odds
                // If you summon an evil mob, it will attack you with 75% odds
                if ((IS_EVIL(victim) && number_bits(2) != 0)
                ||  (IS_NEUTRAL(victim) && number_bits(IS_GOOD(ch) ? 2 : 1) == 0)
                ||  (IS_GOOD(victim) && IS_EVIL(ch)))
                {
                    act("$n roars in fury and attacks!", victim, NULL, NULL, TO_ROOM);
                    one_hit(victim, ch, TYPE_UNDEFINED, HIT_PRIMARY);
                }
            }
        }
    }

    reset();
}

void PyreInfo::checkHeartSacrifice(CHAR_DATA * ch, OBJ_DATA * obj, AFFECT_DATA * heartAff)
{
    // Make sure this is a golden flames PC who does not already have the bit set
    if (ch->pcdata == NULL || testBit(ch, Bit_SacrificedHeartOfInferno) || ch->pcdata->chosen_path != PATH_GOLDENFLAMES)
        return;

    // Test for sufficient strength
    if (heartAff->modifier < 3)
    {
        act("As you sacrifice $p, you feel a certain hunger fill your fiery senses, as though the Inferno wants something...more.", ch, obj, NULL, TO_CHAR);
        return;
    }

    // Test is good; set the bit
    setBit(ch, Bit_SacrificedHeartOfInferno);
    act("As you sacrifice $p, an exultant sense of power fills you as flames momentarily cloud your vision!", ch, obj, NULL, TO_CHAR);
    if (ch->race == global_int_race_shuddeni)
        act("As $n sacrifices $p, $e is surrounded momentarily by a powerful aura of heat!", ch, obj, NULL, TO_ROOM);
    else
        act("As $n sacrifices $p, $s eyes flash red for a moment as $e emanates a sense of fiery power!", ch, obj, NULL, TO_ROOM);
}
