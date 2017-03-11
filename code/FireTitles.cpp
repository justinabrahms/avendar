#include "FireTitles.h"
#include <sstream>

const char * FireTitles::FallbackTitle("the Scholar of Fire");

const char * FireTitles::LookupTitle(const CHAR_DATA & ch)
{
    // Sanity checks
    if (ch.pcdata == NULL || ch.pcdata->major_sphere != SPH_FIRE)
    {
        std::ostringstream mess;
        mess << "Requested fire title on NPC or non-fire PC '" << ch.name << "'; using fallback";
        bug(mess.str().c_str(), 0);
        return FallbackTitle;
    }

    // Subdivide according to minor
    const char * result(NULL);
    bool female(ch.sex == SEX_FEMALE);
    switch (ch.pcdata->minor_sphere)
    {
        case SPH_WATER: result = LookupWaterMinor(ch.level, female);    break;
        case SPH_EARTH: result = LookupEarthMinor(ch.level, female);    break;
        case SPH_VOID:  result = LookupVoidMinor(ch.level, female);     break;
        case SPH_SPIRIT: result = LookupSpiritMinor(ch.level, female);  break;
        case SPH_AIR:   result = LookupAirMinor(ch.level, female);      break;
        case SPH_FIRE:  
            switch (ch.pcdata->chosen_path)
            {
                // Subdivide according to path
                case PATH_FLAMEHEART:   result = LookupFlameheart(ch.level, female);    break;
                case PATH_GOLDENFLAMES: result = LookupGoldenFlames(ch.level, female);  break;
                case PATH_RAGINGINFERNO: result = LookupRagingInferno(ch.level, female); break;
            }
            break;
    }

    // If a title has been found, bail out
    if (result != NULL)
        return result;

    // No title yet, use the default
    result = LookupDefault(ch.level, female);
    if (result != NULL)
        return result;

    // Use the fallback
    std::ostringstream mess;
    mess << "Unable to find fire title for '" << ch.name << "' of level " << ch.level << "; using fallback";
    bug(mess.str().c_str(), 0);
    return FallbackTitle;
}

const char * FireTitles::LookupDefault(int level, bool female)
{
    switch (level)
    {
        case 1:  return "the Neophyte of Fire";
        case 2:  return "the Dabbler of Fire";
        case 3:  return "the Novice of Fire";
        case 4:  return "the Acolyte of Fire";
        case 5:  return "the Pupil of Fire";
        case 6:  return "the Student of Fire";
        case 7:  return "the Follower of Fire";
        case 8:  return "the Seeker of Fire";
        case 9:  return "the Apprentice of Fire";
        case 10: return "the Scholar of Fire";
        case 11: return "the Researcher of Fire";
        case 12: return "the Imbued of Fire";
        case 13: return "the Devoted of Fire";
        case 14: return "the Disciple of Fire";
        case 15: return "the Gifted of Fire";
        case 16: return "the Enlightened of Fire";
        case 17: return "the Aspirant of Fire";
        case 18: return "the Second Rank Journeyman of Fire";
        case 19: return "the Journeyman of Fire";
        case 20: return "the Graduate of Fire";
        case 21: return "the Lesser Magician of Fire";
        case 22: return "the Magician of Fire";
        case 23: return "the Greater Magician of Fire";
        case 24: return "the Philosopher of Fire";
        case 25: return (female ? "the Sister of Fire" : "the Brother of Fire");
        case 26: return "the Accomplished of Fire";
        case 27: return "the Transmuter of Fire";
        case 28: return "the Evoker of Fire";
        case 29: return "the Learned of Fire";
        case 30: return "the Mage of Fire";
        case 31: return "the Senior Mage of Fire";
        case 32: return "the Living Flame";
        case 33: return "the Channeler of Fire";
        case 34: return "the Binder of Fire";
        case 35: return (female ? "the Mistress of Fire" : "the Master of Fire");
        case 36: return (female ? "the Grand Mistress of Fire" : "the Grand Master of Fire");
        case 37: return "of the Burning Seal";
        case 38: return "the Searing Fire";
        case 39: return "the Consuming Flame";
        case 40: return "the Unquenchable Blaze";
        case 41: return "the Oracle of Flames";
        case 42: return "the Wielder of Burning Fire";
        case 43: return "the Immolated Soul";
        case 44: return "the Unleasher of Flame";
        case 45: return (female ? "the Mistress of Incandescence" : "the Master of Incandescence");
        case 46: return (female ? "the Mistress of Cinders" : "the Master of Cinders");
        case 47: return (female ? "the Mistress of Embers" : "the Master of Embers");
        case 48: return (female ? "the Mistress of the Unrestrained Blaze" : "the Master of the Unrestrained Blaze");
        case 49: return (female ? "the Mistress of the Omnipresent Flame" : "the Master of the Omnipresent Flame");
        case 50: return "the Pyromancer";
        case 51: return "the Archmage of Fire";
    }

    return NULL;
}

const char * FireTitles::LookupFlameheart(int level, bool female)
{
    switch (level)
    {
        case 30: return "of the Inner Fire";
        case 39: return (female ? "the Mistress of Conflagration" : "the Master of Conflagration");
        case 40: return "the Visage of the Inferno";
        case 42: return "the Summoner of Brimstone";
        case 43: return "the Burning Mind";
        case 46: return "of the Infernal Rhythm";
        case 49: return "the Firebound";
        case 50: return "the Quintessence of Flame";
        case 51: return "the Flameheart";
    }

    return NULL;
}

const char * FireTitles::LookupRagingInferno(int level, bool female)
{
    switch (level)
    {
        case 30: return "the Blazing Heart";
        case 36: return "of the Phoenix Song";
        case 38: return "the Caller of Firestorms";
        case 39: return "the Conduit of Destruction";
        case 40: return "the Furnace of Fury";
        case 42: return "the Summoner of Brimstone";
        case 44: return (female ? "the Mistress of Detonation" : "the Master of Detonation");
        case 48: return "of the Flaming Tempest";
        case 51: return "the Archmage of the Raging Inferno";
    }

    return NULL;
}

const char * FireTitles::LookupGoldenFlames(int level, bool female)
{
    switch (level)
    {
        case 30: return "the Heart of the Inferno";
        case 38: return "the Caller of Firestorms";
        case 40: return "the Enchanter of Blood and Flame";
        case 43: return "the Burning Mind";
        case 49: return "the Lambent One";
        case 51: return "the Archmage of the Golden Flames";
    }

    return NULL;
}

const char * FireTitles::LookupWaterMinor(int level, bool female)
{
    switch (level)
    {
        case 51: return "the Archmage of the Fiery Seas";
    }

    return NULL;
}

const char * FireTitles::LookupEarthMinor(int level, bool female)
{
    switch (level)
    {
        case 51: return "the Archmage of Earthfire";
    }

    return NULL;
}

const char * FireTitles::LookupVoidMinor(int level, bool female)
{
    switch (level)
    {
        case 51: return "the Archmage of Darkfire";
    }

    return NULL;
}

const char * FireTitles::LookupSpiritMinor(int level, bool female)
{
    switch (level)
    {
        case 51: return "the Archmage of the Divine Spark";
    }

    return NULL;
}

const char * FireTitles::LookupAirMinor(int level, bool female)
{
    switch (level)
    {
        case 51: return "the Archmage of the Burning Skies";
    }

    return NULL;
}
