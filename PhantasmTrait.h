#ifndef PHANTASM_TRAIT_H
#define PHANTASM_TRAIT_H

#include <vector>
#include <map>
#include <set>
#include <string>
#include "merc.h"

class PhantasmTrait
{
    static const unsigned int Unlimited = static_cast<unsigned int>(-1);
    static const unsigned int LowHPThreshold = 3000;
    static const unsigned int LowDamageThreshold = 60;

    public:
        enum Trait
        {
            Resilient = 0,  Practiced,      Worldly,
            Swarming,       Plausible,      Natureborn,
            Cityborn,       Substantial,    Protector,
            Guardian,       Overwhelming,   Precise,
            Marksman,       Scapegoat,      Agile,
            Mercurial,      Ferocious,      Dextrous,
            Critical,       Strong,         Mighty,
            Legendary,      Righteous,      Defiling,
            Imbalanced,     Feral,          Civilized,
            Selfless,       Brute,          Vampiric,
            Feeding,        Venomous,       Nightmarish,
            Endless,        Merciless,      Firebreather,
            Max
        };

        struct TakenInfo
        {
            explicit TakenInfo(Trait traitIn);

            Trait trait;
            unsigned int count;
        };

        enum Affinity {Normal = 0, Natural, Unlikely, Absurd};
        typedef std::vector<TakenInfo> TraitList;

        static const std::string & NameFor(Trait trait);
        static const std::string & DescriptionFor(Trait trait);
        static int DrainFor(Trait trait);
        static Trait TraitFor(const char * name);
        static Affinity AffinityFor(Trait trait, const MOB_INDEX_DATA & mobIndex, const TraitList & traits, bool adding);

    private:
        struct Info
        {
            Info(const std::string & nameIn, const std::string & description, int drainIn, unsigned int minCountIn, unsigned int maxCountIn, Affinity lowHPAffinityIn = Normal, Affinity lowDamageAffinityIn = Normal, Affinity evilAffinityIn = Normal, Affinity neutralAffinityIn = Normal, Affinity goodAffinityIn = Normal);
            std::string name;
            std::string description;
            std::set<Trait> prerequisites;
            std::set<Trait> disqualifiers;
            std::map<int, Affinity> racialAffinities;
            std::vector<std::pair<int, Affinity> > actAffinities;
            int drain;
            unsigned int minCount;
            unsigned int maxCount;
            Affinity lowHPAffinity;
            Affinity lowDamageAffinity;
            Affinity evilAffinity;
            Affinity neutralAffinity;
            Affinity goodAffinity;
        };

        static const std::vector<Info> & BuildTraits();
        static const std::vector<Info> & Traits();
};

inline PhantasmTrait::TakenInfo::TakenInfo(Trait traitIn) : trait(traitIn), count(1) {}
inline const std::string & PhantasmTrait::NameFor(Trait trait) {return Traits()[trait].name;}
inline const std::string & PhantasmTrait::DescriptionFor(Trait trait) {return Traits()[trait].description;}
inline int PhantasmTrait::DrainFor(Trait trait) {return Traits()[trait].drain;}

#endif
