#ifndef SOMATICARTSINFO_H
#define SOMATICARTSINFO_H

#include <vector>
#include <string>

class SomaticArtsInfo
{
    struct Info
    {
        explicit Info(int raceIn);

        int race;
        int skill;
    };

    public:
        static const int Unknown = -1;

        int SkillFor(int race) const;
        unsigned int RacesKnown() const;

        void SetRace(int race, int skill);
        void ForgetRace(int race);
        bool CheckImproveRace(int race, bool sameRaceBonus, bool violenceBonus, int currInt);

        std::string Display() const;
        std::string Serialize() const;
        void Deserialize(const char * buffer);

    private:
        // Members
        std::vector<Info> m_infos;

        // Methods
        int Lookup(int race) const;
};

inline SomaticArtsInfo::Info::Info(int raceIn) : race(raceIn), skill(1) {}
inline unsigned int SomaticArtsInfo::RacesKnown() const {return m_infos.size();}

#endif
