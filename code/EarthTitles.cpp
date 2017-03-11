#include "EarthTitles.h"
#include <sstream>

const char * EarthTitles::FallbackTitle("the Scholar of Earth");

const char * EarthTitles::LookupTitle(const CHAR_DATA & ch)
{
    // Sanity checks
    if (ch.pcdata == NULL || ch.pcdata->major_sphere != SPH_EARTH)
    {
        std::ostringstream mess;
        mess << "Requested earth title on NPC or non-earth PC '" << ch.name << "'; using fallback";
        bug(mess.str().c_str(), 0);
        return FallbackTitle;
    }

    // Subdivide according to minor
    const char * result(NULL);
    bool female(ch.sex == SEX_FEMALE);
    switch (ch.pcdata->minor_sphere)
    {
        case SPH_SPIRIT: result = LookupSpiritMinor(ch.level, female);    break;
        case SPH_AIR: result = LookupAirMinor(ch.level, female);    break;
        case SPH_VOID:  result = LookupVoidMinor(ch.level, female);     break;
        case SPH_EARTH: 
            switch (ch.pcdata->chosen_path)
            {
                // Subdivide according to path
                case PATH_STONESHAPER: result = LookupStoneshaper(ch.level, female); break;
                case PATH_GEOMANCER: result = LookupGeomancer(ch.level, female); break;
                case PATH_WAKENEDSTONE: result = LookupWakenedStone(ch.level, female); break;
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
    mess << "Unable to find earth title for '" << ch.name << "' of level " << ch.level << "; using fallback";
    bug(mess.str().c_str(), 0);
    return FallbackTitle;
}

const char * EarthTitles::LookupDefault(int level, bool female)
{
    switch (level)
    {
        case 1:  return "the Neophyte of Earth";
        case 2:  return "the Dabbler of Earth";
        case 3:  return "the Novice of Earth";
        case 4:  return "the Acolyte of Earth";
        case 5:  return "the Pupil of Earth";
        case 6:  return "the Student of Earth";
        case 7:  return "the Follower of Earth";
        case 8:  return "the Seeker of Earth";
        case 9:  return "the Apprentice of Earth";
        case 10: return "the Scholar of Earth";
        case 11: return "the Researcher of Earth";
        case 12: return "the Imbued of Earth";
        case 13: return "the Devoted of Earth";
        case 14: return "the Disciple of Earth";
        case 15: return "the Gifted of Earth";
        case 16: return "the Enlightened of Earth";
        case 17: return "the Aspirant of Earth";
        case 18: return "the Second Rank Journeyman of Earth";
        case 19: return "the Journeyman of Earth";
        case 20: return "the Graduate of Earth";
        case 21: return "the Lesser Magician of Earth";
        case 22: return "the Magician of Earth";
        case 23: return "the Greater Magician of Earth";
        case 24: return "the Philosopher of Earth";
        case 25: return (female ? "the Sister of Earth" : "the Brother of Earth");
        case 26: return "the Accomplished of Earth";
        case 27: return "the Transmuter of Earth";
        case 28: return "the Evoker of Earth";
        case 29: return "the Learned of Earth";
        case 30: return "the Mage of Earth";
        case 31: return "the Senior Mage of Earth";
        case 32: return "the Communer with Earth";
        case 33: return "the Channeler of Earth";
        case 34: return "the Binder of Earth";
        case 35: return (female ? "the Mistress of Earth" : "the Master of Earth");
        case 36: return (female ? "the Grand Mistress of Earth" : "the Grand Master of Earth");
        case 37: return "of the Conjured Stone";
        case 38: return "the Adamantine";
        case 39: return "the Transmuter of Steel";
        case 40: return "of the Desert Sand";
        case 41: return "the Obdurate";
        case 42: return "the Bulwark of Stone";
        case 43: return "the Delver of Caverns";
        case 44: return (female ? "the Sister of Mountains" : "the Brother of Mountains");
        case 45: return (female ? "the Mistress of Shale" : "the Master of Shale");
        case 46: return (female ? "the Mistress of Obsidian" : "the Master of Obsidian");
        case 47: return (female ? "the Mistress of Granite" : "the Master of Granite");
        case 48: return (female ? "the Mistress of Hill and Vale" : "the Master of Hill and Vale");
        case 49: return (female ? "the Mistress of Rock and Crag" : "the Master of Rock and Crag");
        case 50: return "the Immutable";
        case 51: return "the Archmage of Earth";
    }

    return NULL;
}

const char * EarthTitles::LookupStoneshaper(int level, bool female)
{
    switch (level)
    {
        case 30: return "the Engraver of Stone";
        case 34: return "the Chiseled Mind";
        case 45: return (female ? "the Mistress of Entombment" : "the Master of Entombment");
        case 48: return (female ? "the Mistress of Rune and Glyph" : "the Master of Rune and Glyph");
        case 51: return "the Sculptor of Earth and Stone";
    }

    return NULL;
}

const char * EarthTitles::LookupWakenedStone(int level, bool female)
{
    switch (level)
    {
        case 30: return "the Stonewaker";
        case 34: return "the Speaker for the Earth";
        case 40: return "of the Deep Bedrock";
        case 42: return "the Conduit of Earthsong";
        case 50: return "the Haven of Stone";
        case 51: return "the Archmage of Wakened Stone";
    }

    return NULL;
}

const char * EarthTitles::LookupGeomancer(int level, bool female)
{
    switch (level)
    {
        case 40: return "the Crushing Fist";
        case 42: return (female ? "the Sister of Diamond" : "the Brother of Diamond");
        case 50: return "the Salt of the Earth";
        case 51: return "the Geomancer";
    }

    return NULL;
}

const char * EarthTitles::LookupSpiritMinor(int level, bool female)
{
    switch (level)
    {
        case 51: return "the Archmage of the Crystal Soul";
    }

    return NULL;
}

const char * EarthTitles::LookupAirMinor(int level, bool female)
{
    switch (level)
    {
        case 51: return "the Archmage of Fallensky";
    }

    return NULL;
}

const char * EarthTitles::LookupVoidMinor(int level, bool female)
{
    switch (level)
    {
        case 51: return "the Archmage of Shadow and Wheel";
    }

    return NULL;
}

const char * EarthTitles::LookupFireMinor(int level, bool female)
{
    switch (level)
    {
        case 51: return "the Archmage of the Iron Forge";
    }

    return NULL;
}

const char * EarthTitles::LookupWaterMinor(int level, bool female)
{
    switch (level)
    {
        case 51: return "the Archmage of Stone and Spring";
    }

    return NULL;
}
