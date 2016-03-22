#include "VoidTitles.h"
#include <sstream>

const char * VoidTitles::FallbackTitle("the Scholar of Void");

const char * VoidTitles::LookupTitle(const CHAR_DATA & ch)
{
    // Sanity checks
    if (ch.pcdata == NULL || ch.pcdata->major_sphere != SPH_VOID)
    {
        std::ostringstream mess;
        mess << "Requested void title on NPC or non-void PC '" << ch.name << "'; using fallback";
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
        case SPH_EARTH:  result = LookupEarthMinor(ch.level, female);     break;
        case SPH_VOID: 
            switch (ch.pcdata->chosen_path)
            {
                // Subdivide according to path
                case PATH_NECROMANCER: result = LookupNecromancer(ch.level, female); break;
                case PATH_NIGHTFALL: result = LookupNightfall(ch.level, female); break;
                case PATH_RIVENVEIL: result = LookupRivenVeil(ch.level, female); break;
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
    mess << "Unable to find void title for '" << ch.name << "' of level " << ch.level << "; using fallback";
    bug(mess.str().c_str(), 0);
    return FallbackTitle;
}

const char * VoidTitles::LookupDefault(int level, bool female)
{
    switch (level)
    {
        case 1:  return "the Neophyte of Void";
        case 2:  return "the Dabbler of Void";
        case 3:  return "the Novice of Void";
        case 4:  return "the Acolyte of Void";
        case 5:  return "the Pupil of Void";
        case 6:  return "the Student of Void";
        case 7:  return "the Follower of Void";
        case 8:  return "the Seeker of Void";
        case 9:  return "the Apprentice of Void";
        case 10: return "the Scholar of Void";
        case 11: return "the Researcher of Void";
        case 12: return "the Imbued of Void";
        case 13: return "the Devoted of Void";
        case 14: return "the Disciple of Void";
        case 15: return "the Gifted of Void";
        case 16: return "the Enlightened of Void";
        case 17: return "the Aspirant of Void";
        case 18: return "the Second Rank Journeyman of Void";
        case 19: return "the Journeyman of Void";
        case 20: return "the Graduate of Void";
        case 21: return "the Lesser Magician of Void";
        case 22: return "the Magician of Void";
        case 23: return "the Greater Magician of Void";
        case 24: return "the Philosopher of Void";
        case 25: return (female ? "the Sister of Void" : "the Brother of Void");
        case 26: return "the Accomplished of Void";
        case 27: return "the Transmuter of Void";
        case 28: return "the Evoker of Void";
        case 29: return "the Learned of Void";
        case 30: return "the Mage of Void";
        case 31: return "the Senior Mage of Void";
        case 32: return "the Communer with Void";
        case 33: return "the Channeler of Void";
        case 34: return "the Binder of Void";
        case 35: return (female ? "the Mistress of Void" : "the Master of Void");
        case 36: return (female ? "the Grand Mistress of Void" : "the Grand Master of Void");
        case 37: return "the Harbinger of Plague";
        case 38: return "the Herald of Anguish";
        case 39: return (female ? "the Desecratrix" : "the Desecrator");
        case 40: return "the Darkling Soul";
        case 41: return "the Terror of Night";
        case 42: return "the Defiled One";
        case 43: return "the Binder of Skulls";
        case 44: return "the Shadow of Fear";
        case 45: return (female ? "the Mistress of Maleficence" : "the Master of Maleficence");
        case 46: return (female ? "the Mistress of Torment" : "the Master of Torment");
        case 47: return (female ? "the Mistress of Lamentation" : "the Master of Lamentation");
        case 48: return (female ? "the Mistress of Curse and Bane" : "the Master of Curse and Bane");
        case 49: return (female ? "the Mistress of Night and Sorrow" : "the Master of Night and Sorrow");
        case 50: return "the Spectre of Death";
        case 51: return "the Archmage of Void";
    }

    return NULL;
}

const char * VoidTitles::LookupNecromancer(int level, bool female)
{
    switch (level)
    {
        case 30: return "the Seeker of Undeath";
        case 35: return "the Bringer of Death";
        case 40: return "the Revenant";
        case 42: return "the Caller of Graveborn";
        case 43: return "the Animator of Tombs";
        case 44: return "the Sensate of Death";
        case 45: return (female ? "the Mistress of the Soulless" : "the Master of the Soulless");
        case 46: return (female ? "the Mistress of Reaping" : "the Master of Reaping");
        case 47: return (female ? "the Mistress of Corpses" : "the Master of Corpses");
        case 48: return (female ? "the Mistress of the Unholy Feast" : "the Master of the Unholy Feast");
        case 49: return (female ? "the Mistress of the Restless Dead" : "the Master of the Restless Dead");
        case 51: return "the Sovereign of the Damned";
    }

    return NULL;
}

const char * VoidTitles::LookupNightfall(int level, bool female)
{
    switch (level)
    {
        case 30: return "the Seeker of Night";
        case 35: return "the Scion of Night";
        case 40: return "the Hand of Twilight";
        case 42: return "the Enshrouded of Darkness";
        case 45: return (female ? "the Mistress of Gloom" : "the Master of Gloom");
        case 50: return "the Archfiend";
        case 51: return "the Archmage of Fallen Night";
    }

    return NULL;
}

const char * VoidTitles::LookupRivenVeil(int level, bool female)
{
    switch (level)
    {
        case 30: return "the Veilseeker";
        case 40: return "the Summoner of Demons";
        case 45: return (female ? "the Mistress of Binding" : "the Master of Binding");
        case 46: return (female ? "the Mistress of the Veil" : "the Master of the Veil");
        case 47: return (female ? "the Mistress of the Immutable Void" : "the Master of the Immutable Void");
        case 50: return "the Consort of the Outer Gods";
        case 51: return "the Archmage of the Riven Veil";
    }

    return NULL;
}

const char * VoidTitles::LookupWaterMinor(int level, bool female)
{
    switch (level)
    {
        case 30: return "of the Dead Seas";
        case 35: return "the Lament of Sailors";
        case 46: return (female ? "the Mistress of Frost and Shadow" : "the Master of Frost and Shadow");
        case 47: return (female ? "the Mistress of the Watery Grave" : "the Master of the Watery Grave");
        case 48: return (female ? "the Mistress of the Accursed Seas" : "the Master of the Accursed Seas");
        case 49: return (female ? "the Mistress of Blood and Plague" : "the Master of Blood and Plague");
        case 50: return "the Phantom of Winter";
        case 51: return "the Archmage of the Baleful Deeps";
    }

    return NULL;
}

const char * VoidTitles::LookupAirMinor(int level, bool female)
{
    switch (level)
    {
        case 30: return "the Seeker of Dark Fates";
        case 46: return (female ? "the Mistress of Epidemic" : "the Master of Epidemic");
        case 47: return (female ? "the Mistress of Masquerade" : "the Master of Masquerade");
        case 48: return (female ? "the Mistress of Plague Winds" : "the Master of Plague Winds");
        case 49: return (female ? "the Mistress of Illusion and Fear" : "the Master of Illusion and Fear");
        case 50: return "the Raven of Night";
        case 51: return "the Archmage of Darkstorm";
    }

    return NULL;
}

const char * VoidTitles::LookupEarthMinor(int level, bool female)
{
    switch (level)
    {
        case 30: return "the Serpent Speaker";
        case 46: return (female ? "the Mistress of Grave and Tomb" : "the Master of Grave and Tomb");
        case 47: return (female ? "the Mistress of Shadow and Iron" : "the Master of Shadow and Iron");
        case 48: return (female ? "the Mistress of Stone and Fang" : "the Master of Stone and Fang");
        case 49: return (female ? "the Mistress of Strength and Sorrow" : "the Master of Strength and Sorrow");
        case 50: return "of the Desolated Earth";
        case 51: return "the Archmage of the Earthen Crypt";
    }

    return NULL;
}

const char * VoidTitles::LookupSpiritMinor(int level, bool female)
{
    switch (level)
    {
        case 30: return "the Seeker of Night and Day";
        case 45: return (female ? "the Mistress of Shackles" : "the Master of Shackles");
        case 46: return (female ? "the Mistress of Soulbinding" : "the Master of Soulbinding");
        case 47: return (female ? "the Mistress of Grace and Iniquity" : "the Master of Grace and Iniquity");
        case 48: return (female ? "the Mistress of the Fettered Spirit" : "the Master of the Fettered Spirit");
        case 49: return (female ? "the Mistress of the Evergrey" : "the Master of the Evergrey");
        case 50: return "the Devourer of Souls";
        case 51: return "the Archmage of the Penumbra";
    }

    return NULL;
}

const char * VoidTitles::LookupFireMinor(int level, bool female)
{
    switch (level)
    {
        case 30: return "the Tamer of Vixens";
        case 40: return "the Channeler of Pain";
        case 46: return (female ? "the Mistress of Focus and Fury" : "the Master of Focus and Fury");
        case 47: return (female ? "the Mistress of Torture" : "the Master of Torture");
        case 48: return (female ? "the Mistress of Sadism" : "the Master of Sadism");
        case 49: return (female ? "the Mistress of Undying Anguish" : "the Master of Undying Anguish");
        case 50: return "the Thorn of Sorrows";
        case 51: return "the Archmage of Suffering";
    }

    return NULL;
}
