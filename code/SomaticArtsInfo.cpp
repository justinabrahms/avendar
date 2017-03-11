#include "SomaticArtsInfo.h"
#include "merc.h"
#include <sstream>
#include <iomanip>

const int SomaticArtsInfo::Unknown;

int SomaticArtsInfo::SkillFor(int race) const
{
    int index(Lookup(race));
    return (index == Unknown ? Unknown : m_infos[index].skill);
}

void SomaticArtsInfo::SetRace(int race, int skill)
{
    int index(Lookup(race));
    if (index == Unknown)
    {
        Info info(race);
        info.skill = skill;
        m_infos.push_back(info);
        return;
    }

    m_infos[index].skill = skill;
}

void SomaticArtsInfo::ForgetRace(int race)
{
    int index(Lookup(race));
    if (index == Unknown)
        return;

    m_infos[index] = m_infos[m_infos.size() - 1];
    m_infos.pop_back();
}

bool SomaticArtsInfo::CheckImproveRace(int race, bool sameRaceBonus, bool violenceBonus, int currInt)
{
    // Lookup the race
    int index(Lookup(race));
    if (index == Unknown || m_infos[index].skill >= 1000)
        return false;

    // Determine odds
    int chance(1200 - m_infos[index].skill + ((currInt - 20) * 20));
    chance /= 3;
    if (!sameRaceBonus) chance /= 2;
    if (!violenceBonus) chance /= 3;
    if (number_range(1, 10000) > UMAX(chance, 1))
        return false;
    
    // Improve the skill
    ++m_infos[index].skill;
    return true;
}

std::string SomaticArtsInfo::Display() const
{
    if (m_infos.empty())
        return "None\n";

    std::ostringstream result;
    for (size_t i(0); i < m_infos.size(); ++i)
    {
        result << std::setw(10) << std::left << race_table[m_infos[i].race].name << " ";
        result << std::setw(3) << std::right << (m_infos[i].skill / 10) << '.';
        result << std::setw(1) << std::left << (m_infos[i].skill % 10) << "%\n";
    }

    return result.str();
}

std::string SomaticArtsInfo::Serialize() const
{
    std::ostringstream result;
    for (size_t i(0); i < m_infos.size(); ++i)
    {
        if (i != 0)
            result << ' ';

        result << m_infos[i].race << " " << m_infos[i].skill;
    }

    return result.str();
}

void SomaticArtsInfo::Deserialize(const char * buffer)
{
    std::istringstream input(buffer);
    while (!input.eof())
    {
        int race;
        input >> race;
        if (input.eof())
            return;

        Info info(race);
        input >> info.skill;

        if (Lookup(race) == Unknown)
            m_infos.push_back(info);
    }
}

int SomaticArtsInfo::Lookup(int race) const
{
    for (size_t i(0); i < m_infos.size(); ++i)
    {
        if (m_infos[i].race == race)
            return static_cast<int>(i);
    }

    return Unknown;
}
