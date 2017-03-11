#include "WaterTitles.h"
#include <sstream>

const char * WaterTitles::FallbackTitle("the Scholar of Water");

const char * WaterTitles::LookupTitle(const CHAR_DATA & ch)
{
    // Sanity checks
    if (ch.pcdata == NULL || ch.pcdata->major_sphere != SPH_WATER)
    {
        std::ostringstream mess;
        mess << "Requested water title on NPC or non-water PC '" << ch.name << "'; using fallback";
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
        case SPH_WATER: 
            switch (ch.pcdata->chosen_path)
            {
                // Subdivide according to path
                case PATH_LIVINGWATERS: result = LookupLivingWaters(ch.level, female); break;
                case PATH_WAVEBORNE: result = LookupWaveborne(ch.level, female); break;
                case PATH_WINTERTIDE: result = LookupWintertide(ch.level, female); break;
            }
            break;

        case SPH_AIR:   result = LookupAirMinor(ch.level, female);      break;
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
    mess << "Unable to find water title for '" << ch.name << "' of level " << ch.level << "; using fallback";
    bug(mess.str().c_str(), 0);
    return FallbackTitle;
}

const char * WaterTitles::LookupDefault(int level, bool female)
{
    switch (level)
    {
        case 1:  return "the Neophyte of Water";
        case 2:  return "the Dabbler of Water";
        case 3:  return "the Novice of Water";
        case 4:  return "the Acolyte of Water";
        case 5:  return "the Pupil of Water";
        case 6:  return "the Student of Water";
        case 7:  return "the Follower of Water";
        case 8:  return "the Seeker of Water";
        case 9:  return "the Apprentice of Water";
        case 10: return "the Scholar of Water";
        case 11: return "the Researcher of Water";
        case 12: return "the Imbued of Water";
        case 13: return "the Devoted of Water";
        case 14: return "the Disciple of Water";
        case 15: return "the Gifted of Water";
        case 16: return "the Enlightened of Water";
        case 17: return "the Aspirant of Water";
        case 18: return "the Second Rank Journeyman of Water";
        case 19: return "the Journeyman of Water";
        case 20: return "the Graduate of Water";
        case 21: return "the Lesser Magician of Water";
        case 22: return "the Magician of Water";
        case 23: return "the Greater Magician of Water";
        case 24: return "the Philosopher of Water";
        case 25: return (female ? "the Sister of Water" : "the Brother of Water");
        case 26: return "the Accomplished of Water";
        case 27: return "the Transmuter of Water";
        case 28: return "the Evoker of Water";
        case 29: return "the Learned of Water";
        case 30: return "the Mage of Water";
        case 31: return "the Senior Mage of Water";
        case 32: return "the Ward of Hosts";
        case 33: return "the Channeler of Water";
        case 34: return "the Binder of Water";
        case 35: return (female ? "the Mistress of Water" : "the Master of Water");
        case 36: return (female ? "the Grand Mistress of Water" : "the Grand Master of Water");
        case 37: return "the Mender of Wounds";
        case 38: return "the Caller of Floods";
        case 39: return "the Voice of the Deep";
        case 40: return "the Healing Hand";
        case 41: return "the Breath of Frost";
        case 42: return (female ? "the Prophetess of Tides" : "the Prophet of Tides");
        case 43: return "the Wellspring of Life";
        case 44: return "the Summoner of Ice";
        case 45: return (female ? "the Mistress of Respite" : "the Master of Respite");
        case 46: return (female ? "the Mistress of Frost" : "the Master of Frost");
        case 47: return (female ? "the Mistress of Restoration" : "the Master of Restoration");
        case 48: return (female ? "the Mistress of the Sapphire Shoals" : "the Master of the Sapphire Shoals");
        case 49: return (female ? "the Mistress of the Fallen Snow" : "the Master of the Fallen Snow");
        case 50: return "the Vivimancer";
        case 51: return "the Archmage of Water";
    }

    return NULL;
}

const char * WaterTitles::LookupLivingWaters(int level, bool female)
{
    switch (level)
    {
        case 41: return "the Physiker";
        case 51: return "the Archmage of the Living Waters";
    }

    return NULL;
}

const char * WaterTitles::LookupWaveborne(int level, bool female)
{
    switch (level)
    {
        case 41: return "the Conjurer of Maelstroms";
        case 45: return (female ? "the Mistress of Fathoms" : "the Master of Fathoms");
        case 50: return "the Floodbringer";
        case 51: return "the Archmage of the Waves";
    }

    return NULL;
}

const char * WaterTitles::LookupWintertide(int level, bool female)
{
    switch (level)
    {
        case 38: return "the Herald of Winter";
        case 40: return "the Subthermal";
        case 50: return "the Hand of Winter";
        case 51: return "the Archmage of Winter";
    }

    return NULL;
}

const char * WaterTitles::LookupSpiritMinor(int level, bool female)
{
    switch (level)
    {
        case 39: return "the Clarified Mind";
        case 51: return "the Archmage of Resurrection";
    }

    return NULL;
}

const char * WaterTitles::LookupEarthMinor(int level, bool female)
{
    switch (level)
    {
        case 40: return "the Sanctum";
        case 50: return "the Caller of Permafrost";
        case 51: return "the Archmage of Ice and Stone";
    }

    return NULL;
}

const char * WaterTitles::LookupVoidMinor(int level, bool female)
{
    switch (level)
    {
        case 50: return "the Chill of Night";
        case 51: return "the Archmage of Winter's Night";
    }

    return NULL;
}

const char * WaterTitles::LookupFireMinor(int level, bool female)
{
    switch (level)
    {
        case 40: return "the Boiler of Seas";
        case 51: return "the Archmage of Frost and Flame";
    }

    return NULL;
}

const char * WaterTitles::LookupAirMinor(int level, bool female)
{
    switch (level)
    {
        case 40: return "the Summoner of Rains";
        case 46: return (female ? "the Mistress of Fog" : "the Master of Fog");
        case 51: return "the Archmage of Storms";
    }

    return NULL;
}
