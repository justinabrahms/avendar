#ifndef EXPERIENCELIST_H
#define EXPERIENCELIST_H

#include "merc.h"
#include <vector>
#include <ctime>

class ExperienceList
{
    /// Constants
    static const int DeleteTimeSeconds = 10 * 60;

    /// Types
    struct Info
    {
        Info(long idIn, int amountIn);

        long id;
        int amount;
        time_t lastHit;          
    };

    struct Group
    {
        Group();

        std::vector<CHAR_DATA *> people;
        int amount;
    };

    public:
        void OnDamage(const CHAR_DATA & ch, int amount);
        void OnDeath(const CHAR_DATA & victim);

    private:
        /// Implementation
        void Clean();
        void AddToGroup(Group & group, CHAR_DATA & ch) const;
        static int ComputeXP(const CHAR_DATA & ch, const CHAR_DATA & victim, unsigned int groupSize);

        /// Members
        std::vector<Info> m_infos;
};

#endif
