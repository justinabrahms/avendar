#ifndef FORGE_H
#define FORGE_H

#include "merc.h"
#include <map>
#include <vector>

class Forge
{
    static const unsigned int GuaranteedRange = static_cast<unsigned int>(-1);
    static const int NamingManaCost = 100;

    public:
        enum Trait 
        {
            Trait_Parry = 0,    Trait_Casting,  Trait_Water,
            Trait_Earth,        Trait_Void,     Trait_Spirit,
            Trait_Air,          Trait_Fire
        };

        enum Weapon
        {
            Rapier = 0,     Shortsword,     Broadsword,
            Claymore,       Quarterstaff,   Staff,
            Mace,           Warhammer,      Morningstar,
            Javelin,        Spear,          Dirk,
            Dagger,         Halberd,        Lucerne,
            Hatchet,        Battleaxe,      Fallenstar,
            Flail,          Whip
        };

        static void NameWeapon(CHAR_DATA & ch, const char * argument);
        static void CreateWeapon(CHAR_DATA & ch, const char * argument);
        static unsigned int TraitCount(const OBJ_DATA & obj, Trait trait);

    private:
        /// Types
        typedef void (*AddSpecialCallback)(OBJ_DATA & obj);

        enum Special
        {
            Parry,          Casting,        NoDisarm,
            Vampiric,       Poison,         Hitroll,
            Damroll,        Saves,          Luck,           
            HitPoints,      Mana,           Moves,
            Water,          Earth,          Void,
            Spirit,         Air,            Fire
        };

        struct SpecialInfo
        {
            SpecialInfo(Special specialIn, int baseOddsIn, int costIn, bool multIn, bool uniqueClassIn, AddSpecialCallback callbackIn);

            Special special;
            int baseOdds;
            int cost;
            bool multipleAllowed;
            bool uniqueClass;
            AddSpecialCallback callback;
        };

        enum Bias {Prohibited = 0, Unlikely, Normal, Likely, Guaranteed};

        struct AffinityInfo
        {
            AffinityInfo(Special specialIn, Bias biasIn);

            Special special;
            Bias bias;
        };

        typedef std::map<Special, AffinityInfo> AffinityMap;
        typedef std::vector<AffinityInfo> AffinityList;
        typedef std::vector<std::string> WordList;

        struct WeaponInfo
        {
            Weapon weapon;
            int type;
            std::string name;
            bool twohanded;
            int damtype;
            int dammod;
            int minWeight;
            int objSize;
            AffinityMap affinities;
            WordList damverbs;
            WordList adjectives;
            WordList sentences;
        };

        struct ForgeContext
        {
            ForgeContext();

            int weight;
            int skill;
            int flaws;
            bool named;
            const WeaponInfo * weaponInfo;
            std::vector<int> materials;
        };
        
        typedef std::map<Weapon, WeaponInfo> WeaponMap;
        typedef std::map<Special, SpecialInfo> SpecialMap;
        typedef std::map<int, AffinityList> MaterialMap;

        // Methods
        static const WordList & BuildAdjectives();
        static const WordList & Adjectives();
        static const SpecialMap & BuildSpecials();
        static const SpecialMap & Specials();
        static const WeaponMap & BuildWeapons();
        static const WeaponMap & Weapons();
        static const MaterialMap & BuildMaterials();
        static const MaterialMap & Materials();

        static void CompleteWeapon(CHAR_DATA & ch, ForgeContext & context);
        static bool ObtainBaseObject(ForgeContext & context, std::vector<OBJ_DATA *> & baseObjects, CHAR_DATA & ch, const char *& argument);
        static unsigned int CalculateOddsRange(const SpecialInfo & specialInfo, Weapon weapon, const std::vector<int> & materials);
        static Bias DetermineWeaponBias(Special special, Weapon weapon);
        static Bias DetermineMaterialBias(Special special, int material);
        static Bias ResolveBiases(Bias lhs, Bias rhs);
        static const WeaponInfo * LookupWeapon(const char * name);
        static void SetBasicDescs(OBJ_DATA & obj, const char * shortDesc, bool addComma);
        static AFFECT_DATA * FindNameEffect(const CHAR_DATA & ch);
        static OBJ_DATA * FindNameableObject(const CHAR_DATA & ch, const AFFECT_DATA & nameAffect);
};

inline Forge::ForgeContext::ForgeContext() : weight(0), skill(0), flaws(0), named(false), weaponInfo(NULL) {}

#endif
