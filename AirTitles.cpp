#include "AirTitles.h"
#include <sstream>

const char * AirTitles::FallbackTitle("the Scholar of Air");

const char * AirTitles::LookupTitle(const CHAR_DATA & ch)
{
    // Sanity checks
    if (ch.pcdata == NULL || ch.pcdata->major_sphere != SPH_AIR)
    {
        std::ostringstream mess;
        mess << "Requested air title on NPC or non-air PC '" << ch.name << "'; using fallback";
        bug(mess.str().c_str(), 0);
        return FallbackTitle;
    }

    // Subdivide according to minor
    const char * result(NULL);
    bool female(ch.sex == SEX_FEMALE);
    switch (ch.pcdata->minor_sphere)
    {
        case SPH_SPIRIT: result = LookupSpiritMinor(ch.level, female);    break;
        case SPH_EARTH: result = LookupEarthMinor(ch.level, female);    break;
        case SPH_VOID:  result = LookupVoidMinor(ch.level, female);     break;
        case SPH_AIR: 
            switch (ch.pcdata->chosen_path)
            {
                // Subdivide according to path
                case PATH_ENDLESSFACADE: result = LookupEndlessFacade(ch.level, female); break;
                case PATH_FLICKERINGSKIES: result = LookupFlickeringSkies(ch.level, female); break;
                case PATH_WINDRIDER: result = LookupWindrider(ch.level, female); break;
            }
            break;

        case SPH_WATER:   result = LookupWaterMinor(ch.level, female);      break;
        case SPH_FIRE:  result = LookupFireMinor(ch.level, female);     break;
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
    mess << "Unable to find air title for '" << ch.name << "' of level " << ch.level << "; using fallback";
    bug(mess.str().c_str(), 0);
    return FallbackTitle;
}

const char * AirTitles::LookupDefault(int level, bool female)
{
    switch (level)
    {
        case 1:  return "the Neophyte of Air";
        case 2:  return "the Dabbler of Air";
        case 3:  return "the Novice of Air";
        case 4:  return "the Acolyte of Air";
        case 5:  return "the Pupil of Air";
        case 6:  return "the Student of Air";
        case 7:  return "the Follower of Air";
        case 8:  return "the Seeker of Air";
        case 9:  return "the Apprentice of Air";
        case 10: return "the Scholar of Air";
        case 11: return "the Researcher of Air";
        case 12: return "the Imbued of Air";
        case 13: return "the Devoted of Air";
        case 14: return "the Disciple of Air";
        case 15: return "the Gifted of Air";
        case 16: return "the Enlightened of Air";
        case 17: return "the Aspirant of Air";
        case 18: return "the Second Rank Journeyman of Air";
        case 19: return "the Journeyman of Air";
        case 20: return "the Graduate of Air";
        case 21: return "the Lesser Magician of Air";
        case 22: return "the Magician of Air";
        case 23: return "the Greater Magician of Air";
        case 24: return "the Philosopher of Air";
        case 25: return (female ? "the Sister of Air" : "the Brother of Air");
        case 26: return "the Accomplished of Air";
        case 27: return "the Transmuter of Air";
        case 28: return "the Evoker of Air";
        case 29: return "the Learned of Air";
        case 30: return "the Mage of Air";
        case 31: return "the Senior Mage of Air";
        case 32: return "the Englamoured One";
        case 33: return "the Channeler of Air";
        case 34: return "the Binder of Air";
        case 35: return (female ? "the Mistress of Air" : "the Master of Air");
        case 36: return (female ? "the Grand Mistress of Air" : "the Grand Master of Air");
        case 37: return (female ? "the Kinswoman of Whirlwinds" : "the Kinsman of Whirlwinds");
        case 38: return "the Master Illusionist";
        case 39: return "the Sojourner of Winds";
        case 40: return "the Caller of Lightning";
        case 41: return "the Crafter of Delusions";
        case 42: return "the Cloud Dancer";
        case 43: return "the Summoner of Sparks";
        case 44: return "the Herald of Hurricanes";
        case 45: return (female ? "the Mistress of Zephyrs" : "the Master of Zephyrs");
        case 46: return (female ? "the Mistress of Fate" : "the Master of Fate");
        case 47: return (female ? "the Mistress of Misdirection" : "the Master of Misdirection");
        case 48: return (female ? "the Mistress of the Roaring Gale" : "the Master of the Roaring Gale");
        case 49: return (female ? "the Mistress of Unbridled Chaos" : "the Master of Unbridled Chaos");
        case 50: return "the Howling Wind";
        case 51: return "the Archmage of Air";
    }

    return NULL;
}

const char * AirTitles::LookupEndlessFacade(int level, bool female)
{
    switch (level)
    {
        case 40: return "the Conjurer of Phantasms";
        case 50: return "the Unseen";
        case 51: return "the Archmage of the Endless Facade";
    }

    return NULL;
}

const char * AirTitles::LookupFlickeringSkies(int level, bool female)
{
    switch (level)
    {
        case 40: return "the Conduit of the Skies";
        case 50: return (female ? "the Wizardess of Lightning" : "the Wizard of Lightning");
        case 51: return "the Archmage of the Flickering Skies";
    }

    return NULL;
}

const char * AirTitles::LookupWindrider(int level, bool female)
{
    switch (level)
    {
        case 40: return "the Hand of Fate";
        case 50: return "of the Four Winds";
        case 51: return "the Rider of the Winds";
    }

    return NULL;
}

const char * AirTitles::LookupSpiritMinor(int level, bool female)
{
    switch (level)
    {
        case 50: return "the Spirit of the Wind";
        case 51: return "the Archmage of the Dancing Lights";
    }

    return NULL;
}

const char * AirTitles::LookupEarthMinor(int level, bool female)
{
    switch (level)
    {
        case 51: return "the Archmage of Wind and Stone";
    }

    return NULL;
}

const char * AirTitles::LookupVoidMinor(int level, bool female)
{
    switch (level)
    {
        case 51: return "the Archmage of the Baleful Wind";
    }

    return NULL;
}

const char * AirTitles::LookupFireMinor(int level, bool female)
{
    switch (level)
    {
        case 51: return "the Archmage of Thunder and Flame";
    }

    return NULL;
}

const char * AirTitles::LookupWaterMinor(int level, bool female)
{
    switch (level)
    {
        case 46: return "the Rainmaker";
        case 50: return "the Breath of Life";
        case 51: return "the Archmage of Effervescence";
    }

    return NULL;
}
