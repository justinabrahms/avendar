#include "PhantasmTrait.h"

PhantasmTrait::Info::Info(const std::string & nameIn, const std::string & descriptionIn, 
                          int drainIn, unsigned int minCountIn, unsigned int maxCountIn,
                          Affinity lowHPAffinityIn, Affinity lowDamageAffinityIn,
                          Affinity evilAffinityIn, Affinity neutralAffinityIn, Affinity goodAffinityIn) :
    name(nameIn), description(descriptionIn), drain(drainIn), minCount(minCountIn), maxCount(maxCountIn),
    lowHPAffinity(lowHPAffinityIn), lowDamageAffinity(lowDamageAffinityIn),
    evilAffinity(evilAffinityIn), neutralAffinity(neutralAffinityIn), goodAffinity(goodAffinityIn)
{}

const std::vector<PhantasmTrait::Info> & PhantasmTrait::BuildTraits()
{
    static std::vector<PhantasmTrait::Info> traits;

    #define ADD_RACE(race_name, affinity) (traits[traits.size() - 1].racialAffinities[race_lookup(race_name)] = affinity)
    #define ADD_ACT(act_bit, affinity) (traits[traits.size() - 1].actAffinities.push_back(std::make_pair(act_bit, affinity)))
    #define ADD_PREREQ(trait_prereq) (traits[traits.size() - 1].prerequisites.insert(trait_prereq))
    #define ADD_DISQ(trait_disqualifier) (traits[traits.size() - 1].disqualifiers.insert(trait_disqualifier))
    
    traits.push_back(Info("Resilient", "Reduces the strain of maintaining the phantasm upon receiving damage", 3, 0, 4, Unlikely));
        ADD_ACT(ACT_WIMPY, Unlikely);
        ADD_RACE("alatharya", Natural);     ADD_RACE("chaja", Natural);         ADD_RACE("dragon", Natural);
        ADD_RACE("bear", Natural);          ADD_RACE("giant", Natural);
        ADD_RACE("aelin", Unlikely);        ADD_RACE("shuddeni", Unlikely);     ADD_RACE("ch'taren", Unlikely);
        ADD_RACE("bat", Absurd);            ADD_RACE("fox", Absurd);            ADD_RACE("rabbit", Absurd);
        ADD_RACE("song bird", Absurd);

    traits.push_back(Info("Practiced", "Reduces the strain of maintaining the phantasm over time (mininum: 4)", -4, 1, Unlimited));

    traits.push_back(Info("Worldly", "Removes disbelief of phantasms of different races", 2, 4, 1));
        ADD_ACT(ACT_UNDEAD, Unlikely);      ADD_ACT(ACT_BADASS, Unlikely);
        ADD_RACE("human", Natural);
        ADD_RACE("unique", Unlikely);       ADD_RACE("seraph", Unlikely);       ADD_RACE("celestial", Unlikely);
        ADD_RACE("demon", Unlikely);        ADD_RACE("generic", Unlikely);      ADD_RACE("spirit", Unlikely);
        ADD_RACE("modron", Absurd);         ADD_RACE("dragon", Absurd);         ADD_RACE("wyvern", Absurd);

    traits.push_back(Info("Swarming", "Reduces disbelief of multiple phantasms", 2, 4, 1));
        ADD_ACT(ACT_UNDEAD, Natural);
        ADD_ACT(ACT_BADASS, Unlikely);
        ADD_RACE("bat", Natural);           ADD_RACE("goblin", Natural);
        ADD_RACE("dragon", Unlikely);       ADD_RACE("bear", Unlikely);

    traits.push_back(Info("Plausible", "Reduces disbelief of phantasm outside its native area", 3, 0, 1));
        ADD_ACT(ACT_BADASS, Unlikely);
        ADD_ACT(ACT_UNDEAD, Absurd);
        ADD_RACE("rabbit", Natural);        ADD_RACE("song bird", Natural);     ADD_RACE("wolf", Natural);
        ADD_RACE("human", Natural);         ADD_RACE("aelin", Natural);         ADD_RACE("caladaran", Natural);
        ADD_RACE("kankoran", Natural);      ADD_RACE("srryn", Natural);         ADD_RACE("ethron", Natural);
        ADD_RACE("alatharya", Natural);     ADD_RACE("nefortu", Natural);       ADD_RACE("chaja", Natural);
        ADD_RACE("dog", Natural);           ADD_RACE("fido", Natural);          ADD_RACE("cat", Natural);
        ADD_RACE("lizard", Natural);        ADD_RACE("pig", Natural);           ADD_RACE("snake", Natural);
        ADD_RACE("water fowl", Natural);
        ADD_RACE("demon", Unlikely);        ADD_RACE("wyvern", Unlikely);       ADD_RACE("unique", Unlikely);
        ADD_RACE("seraph", Unlikely);       ADD_RACE("celestial", Unlikely);
        ADD_RACE("centipede", Absurd);      ADD_RACE("school monster", Absurd); ADD_RACE("dragon", Absurd);
        ADD_RACE("spirit", Absurd);

    traits.push_back(Info("Natureborn", "Reduces disbelief in natural environments; increases it in civilization", 2, 0, 1));
        ADD_PREREQ(Plausible);
        ADD_DISQ(Cityborn);
        ADD_ACT(ACT_ANIMAL, Natural);
        ADD_ACT(ACT_UNDEAD, Absurd);
        ADD_RACE("bat", Natural);           ADD_RACE("centipede", Natural);     ADD_RACE("fox", Natural);
        ADD_RACE("kankoran", Natural);      ADD_RACE("ethron", Natural);        ADD_RACE("drow", Natural);
        ADD_RACE("elf", Natural);           ADD_RACE("bear", Natural);          ADD_RACE("lizard", Natural);
        ADD_RACE("snake", Natural);         ADD_RACE("rabbit", Natural);        ADD_RACE("water fowl", Natural);
        ADD_RACE("song bird", Natural);     ADD_RACE("wolf", Natural);
        ADD_RACE("aelin", Unlikely);
        ADD_RACE("celestial", Absurd);      ADD_RACE("seraph", Absurd);
        ADD_RACE("demon", Absurd);          ADD_RACE("unique", Absurd);         ADD_RACE("modron", Absurd);
    
    traits.push_back(Info("Cityborn", "Reduces disbelief in civilization; increases it in nature", 2, 0, 1));
        ADD_PREREQ(Plausible);
        ADD_DISQ(Natureborn);
        ADD_ACT(ACT_ANIMAL, Absurd);
        ADD_RACE("aelin", Natural);
        ADD_RACE("bat", Unlikely);          ADD_RACE("fox", Unlikely);          ADD_RACE("troll", Unlikely);
        ADD_RACE("kankoran", Unlikely);     ADD_RACE("bear", Unlikely);         ADD_RACE("lizard", Unlikely);
        ADD_RACE("water fowl", Unlikely);   ADD_RACE("giant", Unlikely);        ADD_RACE("rabbit", Unlikely);
        ADD_RACE("wolf", Unlikely);
        ADD_RACE("unique", Absurd);         ADD_RACE("seraph", Absurd);         ADD_RACE("celestial", Absurd);
        ADD_RACE("centipede", Absurd);      ADD_RACE("dragon", Absurd);         ADD_RACE("wyvern",  Absurd);
        ADD_RACE("demon", Absurd);

    traits.push_back(Info("Substantial", "Increases chance of paranoia when disbelief fails", 2, 4, 1));
        ADD_PREREQ(Plausible);
        ADD_RACE("unique", Unlikely);       ADD_RACE("dragon", Unlikely);
        ADD_RACE("seraph", Absurd);         ADD_RACE("celestial", Absurd);      ADD_RACE("demon", Absurd);
        ADD_RACE("spirit", Absurd);

    traits.push_back(Info("Protector", "Grants some facility with the rescue skill", 2, 0, 1));
        ADD_ACT(ACT_AGGRESSIVE, Unlikely);
        ADD_ACT(ACT_UNDEAD, Absurd);
        ADD_RACE("dog", Natural);           ADD_RACE("fido", Natural);          ADD_RACE("ch'taren", Natural);
        ADD_RACE("bat", Unlikely);          ADD_RACE("centipede", Unlikely);    ADD_RACE("dragon", Unlikely);
        ADD_RACE("fox", Unlikely);          ADD_RACE("kobold", Unlikely);       ADD_RACE("troll", Unlikely);
        ADD_RACE("wyvern", Unlikely);       ADD_RACE("shuddeni", Unlikely);     ADD_RACE("goblin", Unlikely);
        ADD_RACE("hobgoblin", Unlikely);    ADD_RACE("modron", Unlikely);       ADD_RACE("lizard", Unlikely);
        ADD_RACE("pig", Unlikely);          ADD_RACE("snake", Unlikely);        ADD_RACE("demon", Unlikely);
        ADD_RACE("water fowl", Absurd);     ADD_RACE("song bird", Absurd);      ADD_RACE("rabbit", Absurd);
        ADD_RACE("spirit", Absurd);

    traits.push_back(Info("Guardian", "Grants additional facility with the rescue skill", 2, 4, 1));
        ADD_PREREQ(Protector);
        ADD_ACT(ACT_ANIMAL, Unlikely);
        ADD_ACT(ACT_AGGRESSIVE, Absurd);
        ADD_ACT(ACT_UNDEAD, Absurd);
        ADD_RACE("nefortu", Unlikely);
        ADD_RACE("bat", Absurd);            ADD_RACE("centipede", Absurd);      ADD_RACE("dragon", Absurd);
        ADD_RACE("fox", Absurd);            ADD_RACE("kobold", Absurd);         ADD_RACE("troll", Absurd);
        ADD_RACE("wyvern", Absurd);         ADD_RACE("shuddeni", Absurd);       ADD_RACE("goblin", Absurd);
        ADD_RACE("hobgoblin", Absurd);      ADD_RACE("modron", Absurd);         ADD_RACE("lizard", Absurd);
        ADD_RACE("pig", Absurd);            ADD_RACE("snake", Absurd);          ADD_RACE("demon", Absurd);
        ADD_RACE("water fowl", Absurd);     ADD_RACE("song bird", Absurd);      ADD_RACE("rabbit", Absurd);
        ADD_RACE("spirit", Absurd);

    traits.push_back(Info("Overwhelming", "Increases the effective level of the phantasm", 2, 0, Unlimited, Normal, Unlikely));
        ADD_ACT(ACT_BADASS, Natural);
        ADD_RACE("dragon", Natural);
        ADD_RACE("elemental", Unlikely);

    traits.push_back(Info("Precise", "Increases the hitroll of the phantasm", 2, 0, Unlimited));
        ADD_ACT(ACT_UNDEAD, Unlikely);
        ADD_RACE("aelin", Natural);         ADD_RACE("nefortu", Natural);       ADD_RACE("snake", Natural);
        ADD_RACE("caladaran", Unlikely);
        ADD_RACE("pig", Absurd);

    traits.push_back(Info("Marksman", "Grants a chance of bypassing defenses when attacking", 2, 4, 1));
        ADD_PREREQ(Precise);
        ADD_ACT(ACT_UNDEAD, Absurd);
        ADD_RACE("ethron", Unlikely);       ADD_RACE("alatharya", Unlikely);    ADD_RACE("giant", Unlikely);
        ADD_RACE("bear", Unlikely);
        ADD_RACE("caladaran", Absurd);      ADD_RACE("pig", Absurd);

    traits.push_back(Info("Scapegoat", "Reduces chance of refocusing attack away from the phantasm", 2, 2, 1, Natural));
        ADD_ACT(ACT_BADASS, Unlikely);
        ADD_RACE("caladaran", Natural);     ADD_RACE("dog", Natural);           ADD_RACE("fido", Natural);
        ADD_RACE("elemental", Natural);
        ADD_RACE("dragon", Unlikely);

    traits.push_back(Info("Agile", "Grants some facility with the dodge skill", 3, 4, 1));
        ADD_ACT(ACT_UNDEAD, Absurd);
        ADD_RACE("aelin", Natural);         ADD_RACE("nefortu", Natural);       ADD_RACE("kankoran", Natural);
        ADD_RACE("srryn", Natural);         ADD_RACE("song bird", Natural);     ADD_RACE("water fowl", Natural);
        ADD_RACE("wyvern", Natural);        ADD_RACE("bat", Natural);           ADD_RACE("cat", Natural);
        ADD_RACE("caladaran", Unlikely);

    traits.push_back(Info("Mercurial", "Grants additional facility with the dodge skill", 4, 6, 1));
        ADD_PREREQ(Agile);
        ADD_ACT(ACT_UNDEAD, Absurd);
        ADD_RACE("aelin", Natural);
        ADD_RACE("bear", Unlikely);         ADD_RACE("alatharya", Unlikely);    ADD_RACE("giant", Unlikely);
        ADD_RACE("modron", Unlikely);
        ADD_RACE("caladaran", Absurd);

    traits.push_back(Info("Ferocious", "Grants some facility with the second attack skill", 4, 4, 1, Normal, Unlikely));
        ADD_ACT(ACT_AGGRESSIVE, Natural);
        ADD_RACE("srryn", Natural);         ADD_RACE("kankoran", Natural);      ADD_RACE("wolf", Natural);
        ADD_RACE("bear", Natural);          ADD_RACE("dragon", Natural);
        ADD_RACE("rabbit", Unlikely);       ADD_RACE("song bird", Unlikely);    ADD_RACE("water fowl", Unlikely);
        ADD_RACE("fox", Unlikely);
        ADD_RACE("caladaran", Absurd);

    traits.push_back(Info("Dextrous", "Grants some facility with the second attack skill", 4, 4, 1));
        ADD_ACT(ACT_UNDEAD, Absurd);
        ADD_RACE("aelin", Natural);         ADD_RACE("nefortu", Natural);       ADD_RACE("wolf", Natural);
        ADD_RACE("kankoran", Natural);      ADD_RACE("srryn", Natural);         ADD_RACE("wyvern", Natural);
        ADD_RACE("caladaran", Unlikely);    ADD_RACE("ethron", Unlikely);       ADD_RACE("alatharya", Unlikely);
        ADD_RACE("giant", Unlikely);

    traits.push_back(Info("Critical", "Increases chance of scoring a critical attack for double damage", 1, 0, 4));
        ADD_ACT(ACT_ANIMAL, Absurd);
        ADD_RACE("elemental", Natural);

    traits.push_back(Info("Strong", "Increases the strength of the phantasm's attacks", 2, 0, 3, Normal, Unlikely));
        ADD_ACT(ACT_BADASS, Natural);
        ADD_RACE("alatharya", Natural);     ADD_RACE("giant", Natural);         ADD_RACE("chaja", Natural);
        ADD_RACE("bear", Natural);          ADD_RACE("dragon", Natural);
        ADD_RACE("aelin", Unlikely);        ADD_RACE("nefortu", Unlikely);      ADD_RACE("ch'taren", Unlikely);
        ADD_RACE("shuddeni", Unlikely);     ADD_RACE("rabbit", Unlikely);

    traits.push_back(Info("Mighty", "Significantly increases the strength of the phantasm's attacks", 4, 2, 2, Normal, Absurd));
        ADD_PREREQ(Strong);
        ADD_ACT(ACT_BADASS, Natural);
        ADD_RACE("dragon", Natural);
        ADD_RACE("aelin", Unlikely);        ADD_RACE("bat", Unlikely);
        ADD_RACE("nefortu", Absurd);        ADD_RACE("ch'taren", Absurd);       ADD_RACE("shuddeni", Absurd);     
        ADD_RACE("rabbit", Absurd);

    traits.push_back(Info("Legendary", "Vastly increases the strength of the phantasms's attack", 8, 6, 1, Absurd, Absurd));
        ADD_PREREQ(Mighty);                 ADD_PREREQ(Overwhelming);
        ADD_RACE("aelin", Unlikely);        ADD_RACE("bat", Unlikely);
        ADD_RACE("nefortu", Absurd);        ADD_RACE("ch'taren", Absurd);       ADD_RACE("shuddeni", Absurd);     
        ADD_RACE("rabbit", Absurd);         ADD_RACE("generic", Absurd);

    traits.push_back(Info("Righteous", "Increases damage done against evils; reduces it against others", 3, 2, 1, Normal, Normal, Absurd, Unlikely, Normal));
        ADD_DISQ(Defiling);                 ADD_DISQ(Imbalanced);
        ADD_ACT(ACT_ANIMAL, Unlikely);
        ADD_ACT(ACT_UNDEAD, Absurd);
        ADD_RACE("seraph", Natural);        ADD_RACE("celestial", Natural);     ADD_RACE("ch'taren", Natural);
        ADD_RACE("srryn", Unlikely);        ADD_RACE("nefortu", Unlikely);      ADD_RACE("chaja", Unlikely);
        ADD_RACE("drow", Unlikely);         ADD_RACE("modron", Unlikely);       ADD_RACE("generic", Unlikely);
        ADD_RACE("shuddeni", Absurd);       ADD_RACE("demon", Absurd);

    traits.push_back(Info("Defiling", "Increases damage done against those of good will; reduces it against others", 3, 2, 1, Normal, Normal, Normal, Normal, Absurd));
        ADD_DISQ(Righteous);                ADD_DISQ(Imbalanced);
        ADD_ACT(ACT_ANIMAL, Unlikely);
        ADD_ACT(ACT_UNDEAD, Natural);
        ADD_RACE("shuddeni", Natural);      ADD_RACE("demon", Natural);
        ADD_RACE("ethron", Unlikely);       ADD_RACE("caladaran", Unlikely);
        ADD_RACE("seraph", Absurd);         ADD_RACE("celestial", Absurd);      ADD_RACE("ch'taren", Absurd);

    traits.push_back(Info("Imbalanced", "Increases damage done against neutrals; reduces it against others", 3, 2, 1, Normal, Normal, Normal, Unlikely, Normal));
        ADD_DISQ(Righteous);                ADD_DISQ(Defiling);
        ADD_ACT(ACT_ANIMAL, Unlikely);

    traits.push_back(Info("Feral", "Increases damage done in nature; reduces it in civilization", 3, 2, 1));
        ADD_DISQ(Civilized);
        ADD_ACT(ACT_ANIMAL, Natural);
        ADD_ACT(ACT_UNDEAD, Unlikely);
        ADD_RACE("bat", Natural);           ADD_RACE("fox", Natural);           ADD_RACE("kankoran", Natural);      
        ADD_RACE("elf", Natural);           ADD_RACE("bear", Natural);          ADD_RACE("lizard", Natural);
        ADD_RACE("snake", Natural);         ADD_RACE("rabbit", Natural);        ADD_RACE("water fowl", Natural);
        ADD_RACE("song bird", Natural);     ADD_RACE("wolf", Natural);
        ADD_RACE("aelin", Unlikely);
        ADD_RACE("celestial", Absurd);      ADD_RACE("seraph", Absurd);
        ADD_RACE("demon", Absurd);          ADD_RACE("unique", Absurd);         ADD_RACE("modron", Absurd);
    
    traits.push_back(Info("Civilized", "Increases damage done in civilization; reduces it in nature", 3, 2, 1));
        ADD_DISQ(Feral);
        ADD_ACT(ACT_UNDEAD, Unlikely);
        ADD_ACT(ACT_ANIMAL, Absurd);
        ADD_RACE("kankoran", Unlikely);     ADD_RACE("giant", Unlikely);        ADD_RACE("rabbit", Unlikely);
        ADD_RACE("wolf", Unlikely);
        ADD_RACE("unique", Absurd);         ADD_RACE("seraph", Absurd);         ADD_RACE("celestial", Absurd);
        ADD_RACE("centipede", Absurd);      ADD_RACE("dragon", Absurd);         ADD_RACE("wyvern",  Absurd);
        ADD_RACE("demon", Absurd);

    traits.push_back(Info("Selfless", "Increases chance of phantasms automatically attempting a rescue", 2, 2, 2, Normal, Normal, Unlikely, Normal, Normal));
        ADD_PREREQ(Protector);
        ADD_ACT(ACT_AGGRESSIVE, Unlikely);
        ADD_ACT(ACT_UNDEAD, Absurd);
        ADD_RACE("bat", Unlikely);          ADD_RACE("centipede", Unlikely);    ADD_RACE("dragon", Unlikely);
        ADD_RACE("fox", Unlikely);          ADD_RACE("kobold", Unlikely);       ADD_RACE("troll", Unlikely);
        ADD_RACE("wyvern", Unlikely);       ADD_RACE("goblin", Unlikely);       ADD_RACE("hobgoblin", Unlikely);    
        ADD_RACE("modron", Unlikely);       ADD_RACE("lizard", Unlikely);       ADD_RACE("pig", Unlikely);          
        ADD_RACE("snake", Unlikely);
        ADD_RACE("water fowl", Absurd);     ADD_RACE("song bird", Absurd);      ADD_RACE("rabbit", Absurd);
        ADD_RACE("spirit", Absurd);         ADD_RACE("shuddeni", Absurd);       ADD_RACE("demon", Absurd);

    traits.push_back(Info("Brute", "Grants a chance of phantasm attempting to bash during combat", 2, 2, 1, Unlikely));
        ADD_RACE("srryn", Natural);         ADD_RACE("alatharya", Natural);     ADD_RACE("giant", Natural);
        ADD_RACE("chaja", Natural);         ADD_RACE("bear", Natural);          ADD_RACE("dragon", Natural);
        ADD_RACE("troll", Natural);         ADD_RACE("orc", Natural);
        ADD_RACE("cat", Unlikely);          ADD_RACE("song bird", Unlikely);    ADD_RACE("water fowl", Unlikely);
        ADD_RACE("centipede", Unlikely);    ADD_RACE("bat", Unlikely);          ADD_RACE("fox", Unlikely);
        ADD_RACE("aelin", Absurd);          ADD_RACE("shuddeni", Absurd);       ADD_RACE("ch'taren", Absurd);
        ADD_RACE("nefortu", Absurd);        ADD_RACE("spirit", Absurd);         ADD_RACE("rabbit", Absurd);

    traits.push_back(Info("Vampiric", "Grants a chance of restoring mana from phantasms's attack", 2, 2, 1, Normal, Normal, Normal, Unlikely, Absurd));
        ADD_RACE("demon", Natural);         ADD_RACE("shuddeni", Natural);      ADD_RACE("spirit", Natural);
        ADD_RACE("bat", Natural);
        ADD_RACE("caladaran", Unlikely);    ADD_RACE("generic", Unlikely);
        ADD_RACE("seraph", Absurd);         ADD_RACE("celestial", Absurd);      ADD_RACE("ch'taren", Absurd);

    traits.push_back(Info("Feeding", "Grants a chance of phantasm growing stronger after a kill", 3, 2, 1, Normal, Normal, Normal, Unlikely, Absurd));
        ADD_ACT(ACT_UNDEAD, Natural);
        ADD_RACE("aelin", Unlikely);        ADD_RACE("caladaran", Unlikely);    ADD_RACE("ethron", Unlikely);

    traits.push_back(Info("Venomous", "Grants a chance of poisoning opponents during combat", 3, 2, 1));
        ADD_RACE("centipede", Natural);
        ADD_RACE("human", Unlikely);        ADD_RACE("caladaran", Unlikely);    ADD_RACE("aelin", Unlikely);
        ADD_RACE("ch'taren", Unlikely);
        ADD_RACE("seraph", Absurd);         ADD_RACE("celestial", Absurd);

    traits.push_back(Info("Nightmarish", "Grants a chance of causing an opponent to flee during combat", 2, 2, 1, Unlikely, Unlikely));
        ADD_PREREQ(Overwhelming);
        ADD_ACT(ACT_UNDEAD, Natural);
        ADD_ACT(ACT_WIMPY, Absurd);
        ADD_RACE("dragon", Natural);        ADD_RACE("demon", Natural);         ADD_RACE("spirit", Natural);
        ADD_RACE("caladaran", Unlikely);    ADD_RACE("ethron", Unlikely);
        ADD_RACE("rabbit", Absurd);         ADD_RACE("generic", Absurd);

    traits.push_back(Info("Endless", "Removes the cooldown after conjuring the phantasm", 2, 0, 1));
        ADD_ACT(ACT_BADASS, Unlikely);
        ADD_RACE("elemental", Natural);

    traits.push_back(Info("Merciless", "Increases damage as opponent gets more injured", 4, 4, 1));
        ADD_RACE("shuddeni", Natural);      ADD_RACE("demon", Natural);
        ADD_RACE("ch'taren", Absurd);       ADD_RACE("caladaran", Absurd);

    traits.push_back(Info("Firebreather", "Grants a chance of breathing fire in combat", 4, 4, 1));
        ADD_RACE("dragon", Natural);
        ADD_RACE("lizard", Unlikely);       ADD_RACE("snake", Unlikely);        ADD_RACE("demon", Unlikely);
        ADD_RACE("unique", Absurd);         ADD_RACE("aelin", Absurd);          ADD_RACE("caladaran", Absurd);
        ADD_RACE("seraph", Absurd);         ADD_RACE("bat", Absurd);            ADD_RACE("centipede", Absurd);
        ADD_RACE("fox", Absurd);            ADD_RACE("kobold", Absurd);         ADD_RACE("orc", Absurd);
        ADD_RACE("school monster", Absurd); ADD_RACE("troll", Absurd);          ADD_RACE("celestial", Absurd);
        ADD_RACE("human", Absurd);          ADD_RACE("kankoran", Absurd);       ADD_RACE("shuddeni", Absurd);
        ADD_RACE("srryn", Absurd);          ADD_RACE("ethron", Absurd);         ADD_RACE("drow", Absurd);
        ADD_RACE("bear", Absurd);           ADD_RACE("dog", Absurd);            ADD_RACE("elf", Absurd);
        ADD_RACE("goblin", Absurd);         ADD_RACE("pig", Absurd);            ADD_RACE("water fowl", Absurd);
        ADD_RACE("alatharya", Absurd);      ADD_RACE("ch'taren", Absurd);       ADD_RACE("nefortu", Absurd);
        ADD_RACE("chaja", Absurd);          ADD_RACE("giant", Absurd);          ADD_RACE("cat", Absurd);
        ADD_RACE("doll", Absurd);           ADD_RACE("fido", Absurd);           ADD_RACE("hobgoblin", Absurd);
        ADD_RACE("modron", Absurd);         ADD_RACE("rabbit", Absurd);         ADD_RACE("song bird", Absurd);
        ADD_RACE("wolf", Absurd);           ADD_RACE("generic", Absurd);        ADD_RACE("spirit", Absurd);

    #undef ADD_DISQ
    #undef ADD_PREREQ
    #undef ADD_ACT
    #undef ADD_RACE
    return traits;
}

const std::vector<PhantasmTrait::Info> & PhantasmTrait::Traits()
{
    static const std::vector<PhantasmTrait::Info> & traits(BuildTraits());
    return traits;
}

PhantasmTrait::Trait PhantasmTrait::TraitFor(const char * name)
{
    for (size_t i(0); i < Traits().size(); ++i)
    {
        if (!str_prefix(name, NameFor(static_cast<Trait>(i)).c_str()))
            return static_cast<Trait>(i);
    }

    return Max;
}

PhantasmTrait::Affinity PhantasmTrait::AffinityFor(Trait trait, const MOB_INDEX_DATA & mobIndex, const TraitList & traits, bool adding)
{
    // Get the info
    const Info & info(Traits()[trait]);

    // Check the disqualifiers, simultaneously counting the number of traits
    unsigned int count(0);
    for (size_t i(0); i < traits.size(); ++i)
    {
        // Banned if any disqualifier found
        if (traits[i].count > 0 && info.disqualifiers.find(traits[i].trait) != info.disqualifiers.end())
            return Absurd;

        // Banned if too many of this trait exist
        if ((traits[i].count > info.maxCount || (adding && traits[i].count == info.maxCount)) && traits[i].trait == trait)
            return Absurd;

        count += traits[i].count;
    }

    // Check for min count
    if (count < info.minCount)
        return Absurd;

    // Check the prerequisites
    for (std::set<Trait>::const_iterator iter(info.prerequisites.begin()); iter != info.prerequisites.end(); ++iter)
    {
        bool found(false);
        for (size_t i(0); i < traits.size(); ++i)
        {
            if (traits[i].count > 0 && traits[i].trait == *iter)
            {
                found = true;
                break;
            }
        }

        // Banned if any prerequisite is missing
        if (!found)
            return Absurd;
    }
    
    // Check the race
    Affinity result(Normal);
    std::map<int, Affinity>::const_iterator iter(info.racialAffinities.find(mobIndex.race));
    if (iter != info.racialAffinities.end() && iter->second > result)
        result = iter->second;

    // Check the act code
    for (size_t i(0); i < info.actAffinities.size(); ++i)
    {
        if (IS_SET(mobIndex.act, info.actAffinities[i].first) && info.actAffinities[i].second > result)
            result = info.actAffinities[i].second;
    }

    // Check the alignment
    switch (mobIndex.alignment)
    {
        case ALIGN_GOOD:    if (info.goodAffinity > result) result = info.goodAffinity; break;
        case ALIGN_EVIL:    if (info.evilAffinity > result) result = info.evilAffinity; break;
        default:            if (info.neutralAffinity > result) result = info.neutralAffinity; break;
    }

    // Check for low hp
    unsigned int aveHP((((mobIndex.hit[1] + 1) * mobIndex.hit[0]) / 2) + mobIndex.hit[2]);
    if (aveHP < LowHPThreshold && info.lowHPAffinity > result)
        result = info.lowHPAffinity;

    // Check for low damage
    unsigned int aveDam((((mobIndex.damage[1] + 1) * mobIndex.damage[0]) / 2) + mobIndex.damage[2]);
    if (aveDam < LowDamageThreshold && info.lowDamageAffinity > result)
        result = info.lowDamageAffinity;

    return result;
}
