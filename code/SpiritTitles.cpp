#include "SpiritTitles.h"
#include <sstream>

const char * SpiritTitles::FallbackTitle("the Scholar of Spirit");

const char * SpiritTitles::LookupTitle(const CHAR_DATA & ch)
{
    // Sanity checks
    if (ch.pcdata == NULL || ch.pcdata->major_sphere != SPH_SPIRIT)
    {
        std::ostringstream mess;
        mess << "Requested spirit title on NPC or non-spirit PC '" << ch.name << "'; using fallback";
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
        case SPH_SPIRIT: 
            switch (ch.pcdata->chosen_path)
            {
                // Subdivide according to path
                case PATH_WEAVEDANCER: result = LookupWeavedancer(ch.level, female); break;
                case PATH_SILVERLIGHT: result = LookupSilverLight(ch.level, female); break;
                case PATH_ETERNALDAWN: result = LookupEternalDawn(ch.level, female); break;
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
    mess << "Unable to find spirit title for '" << ch.name << "' of level " << ch.level << "; using fallback";
    bug(mess.str().c_str(), 0);
    return FallbackTitle;
}

const char * SpiritTitles::LookupDefault(int level, bool female)
{
    switch (level)
    {
        case 1:  return "the Neophyte of Spirit";
        case 2:  return "the Dabbler of Spirit";
        case 3:  return "the Novice of Spirit";
        case 4:  return "the Acolyte of Spirit";
        case 5:  return "the Pupil of Spirit";
        case 6:  return "the Student of Spirit";
        case 7:  return "the Follower of Spirit";
        case 8:  return "the Seeker of Spirit";
        case 9:  return "the Apprentice of Spirit";
        case 10: return "the Scholar of Spirit";
        case 11: return "the Researcher of Spirit";
        case 12: return "the Imbued of Spirit";
        case 13: return "the Devoted of Spirit";
        case 14: return "the Disciple of Spirit";
        case 15: return "the Gifted of Spirit";
        case 16: return "the Enlightened of Spirit";
        case 17: return "the Aspirant of Spirit";
        case 18: return "the Second Rank Journeyman of Spirit";
        case 19: return "the Journeyman of Spirit";
        case 20: return "the Graduate of Spirit";
        case 21: return "the Lesser Magician of Spirit";
        case 22: return "the Magician of Spirit";
        case 23: return "the Greater Magician of Spirit";
        case 24: return "the Philosopher of Spirit";
        case 25: return (female ? "the Sister of Spirit" : "the Brother of Spirit");
        case 26: return "the Accomplished of Spirit";
        case 27: return "the Transmuter of Spirit";
        case 28: return "the Evoker of Spirit";
        case 29: return "the Learned of Spirit";
        case 30: return "the Mage of Spirit";
        case 31: return "the Senior Mage of Spirit";
        case 32: return "the Ethereal Voice";
        case 33: return "the Channeler of Spirit";
        case 34: return "the Binder of Spirit";
        case 35: return (female ? "the Mistress of Spirit" : "the Master of Spirit");
        case 36: return (female ? "the Grand Mistress of Spirit" : "the Grand Master of Spirit");
        case 37: return "the Soul of Fellowship";
        case 38: return "the Hand of Divinity";
        case 39: return "the Spirit of Life";
        case 40: return "the Banisher of Darkness";
        case 41: return "the Weavecaller";
        case 42: return "the Proclaimer of Dawn";
        case 43: return "the Liberator of the Spirit";
        case 44: return "the Minister of Spirit";
        case 45: return (female ? "the Mistress of Benevolence" : "the Master of Benevolence");
        case 46: return (female ? "the Mistress of Truth" : "the Master of Truth");
        case 47: return (female ? "the Mistress of Righteousness" : "the Master of Righteousness");
        case 48: return (female ? "the Mistress of the Illuminated Soul" : "the Master of the Illuminated Soul");
        case 49: return (female ? "the Mistress of the Resonating Weave" : "the Master of the Resonating Weave");
        case 50: return "the Divine Beacon";
        case 51: return "the Archmage of Spirit";
    }

    return NULL;
}

const char * SpiritTitles::LookupWeavedancer(int level, bool female)
{
    switch (level)
    {
        case 40: return "the Weavecrafter";
        case 51: return "the Archmage of Quintessence";
    }

    return NULL;
}

const char * SpiritTitles::LookupSilverLight(int level, bool female)
{
    switch (level)
    {
        case 44: return "the Communer of Souls";
        case 50: return "the Voyager of Planes";
        case 51: return "the Archmage of the Silver Light";
    }

    return NULL;
}

const char * SpiritTitles::LookupEternalDawn(int level, bool female)
{
    switch (level)
    {
        case 44: return "of the Beatific Vision";
        case 51: return "the Archmage of Dawn";
    }

    return NULL;
}

const char * SpiritTitles::LookupWaterMinor(int level, bool female)
{
    switch (level)
    {
        case 51: return "the Archmage of Divine Renewal";
    }

    return NULL;
}

const char * SpiritTitles::LookupEarthMinor(int level, bool female)
{
    switch (level)
    {
        case 51: return "the Archmage of Hallowed Earth";
    }

    return NULL;
}

const char * SpiritTitles::LookupVoidMinor(int level, bool female)
{
    switch (level)
    {
        case 38: return "the Master of Dreams";
        case 51: return "the Archmage of Dreams";
    }

    return NULL;
}

const char * SpiritTitles::LookupFireMinor(int level, bool female)
{
    switch (level)
    {
        case 51: return "the Archmage of Inspiration";
    }

    return NULL;
}

const char * SpiritTitles::LookupAirMinor(int level, bool female)
{
    switch (level)
    {
        case 51: return "the Archmage of the Mystic Sky";
    }

    return NULL;
}
