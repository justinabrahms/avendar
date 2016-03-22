#ifndef PHANTASMINFO_H
#define PHANTASMINFO_H

#include <vector>
#include <ostream>
#include "merc.h"
#include "PhantasmTrait.h"

class PhantasmInfo
{
    static const unsigned int MobSlotInfoIndex = 0;
    static const unsigned int MaxCooldown = 30000;
    static const int BaseDrain = 6;
    static const int MinimumDrain = 4;
    static const size_t TraitNameMaxWidth = 12;

    typedef PhantasmTrait::TraitList TraitList;

    struct Info
    {
        int vnum;
        unsigned int promotions;
        TraitList traits;
    };

    public:
        static const unsigned int NotFound = static_cast<unsigned int>(-1);

        unsigned int count() const;
        unsigned int lookupIndexByName(const char * name) const;
        unsigned int lookupIndexByVnum(int vnum) const;
        bool isPhantasmHere(const ROOM_INDEX_DATA & room, unsigned int index) const;
        CHAR_DATA * generatePhantasm(int casterLevel, unsigned int index) const;
        std::string listPossibleEmpowerments(int casterID, unsigned int index) const;
        std::string listPhantasms() const;
        std::string serialize() const;

        void learnPhantasm(int casterID, int vnum, int bonusDuration);
        void forgetPhantasm(CHAR_DATA & ch, CHAR_DATA & victim, unsigned int index);
        bool empowerPhantasm(CHAR_DATA & ch, int level, unsigned int index, const char * argument);

        static void checkReduceCooldown(const CHAR_DATA & ch);
        static PhantasmInfo * deserialize(const char * buffer);
        static unsigned int traitCount(const CHAR_DATA & ch, PhantasmTrait::Trait trait);
        static const MOB_INDEX_DATA * baseMobIndex(const CHAR_DATA & ch);
        static int totalDrainFor(const CHAR_DATA & ch);
        static bool isPhantasmHere(const ROOM_INDEX_DATA & room, int vnum);

    private:
        /// Members
        std::vector<Info> m_phantasms;

        /// Methods
        const Info * lookupVnum(int vnum) const;
        void serializeInfo(std::ostream & out, const Info & info) const;
        void listPhantasm(std::ostream & mess, size_t index) const;

        static unsigned int makeSeed(int casterID, int vnum);
        static const Info * lookupInfo(const CHAR_DATA & ch);
        static unsigned int calculateNextCooldown(unsigned int seed, unsigned int traitSeed, unsigned int promotions, PhantasmTrait::Affinity affinity);
        static int drainFor(PhantasmTrait::Trait trait, PhantasmTrait::Affinity affinity);
        static int totalDrainFor(const Info & info, const MOB_INDEX_DATA & mobIndex);
        static const MOB_INDEX_DATA * lookupMobIndex(int vnum);
        static void copyMobString(char *& destination, const char * source);
        static void adjustPhantasm(CHAR_DATA & mob, PhantasmTrait::Trait trait, unsigned int count);
        static bool deserializeInfo(Info & info, const char * buffer, size_t length);
};

inline unsigned int PhantasmInfo::count() const {return m_phantasms.size();}

#endif
