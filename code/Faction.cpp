#include "Faction.h"
#include <sstream>
#include <fstream>

const char * Faction::Marker_Name("Name");
const char * Faction::Marker_FriendThreshold("RateF");
const char * Faction::Marker_EnemyThreshold("RateE");
const char * Faction::Marker_Flags("Flags");
const char * Faction::Marker_Ally("Ally");
const char * Faction::Marker_Enemy("Oppo");
const char * Faction::Marker_InitClass("InitClass");
const char * Faction::Marker_InitRace("InitRace");
const char * Faction::Marker_InitAltar("InitAltar");
const char * Faction::Marker_InitGender("InitGender");
const char * Faction::Marker_End("End");

Faction::Faction() : m_friendThreshold(700), m_enemyThreshold(-700) {}

bool Faction::Read(std::istream & input)
{
    std::string key;
    unsigned int value;
    int initValue;

    while (true)
    {
        // Get the key and check for terminating conditions
        input >> key;
        if (key == Marker_End) 
            return true;

        if (input.fail() || input.eof())
            break;

        // Read the elements in
        if (key == Marker_Name) 
        {
            // Eat the single space before grabbing the rest of the line
            input.get();
            std::getline(input, m_name); 
            continue;
        }

        if (key == Marker_FriendThreshold) {input >> m_friendThreshold; continue;}
        if (key == Marker_EnemyThreshold) {input >> m_enemyThreshold; continue;}
        if (key == Marker_Flags) {input >> m_flags; continue;}
        if (key == Marker_Ally) {input >> value; m_allies.push_back(value); continue;}
        if (key == Marker_Enemy) {input >> value; m_enemies.push_back(value); continue;}
        if (key == Marker_InitClass) {input >> value; input >> initValue; m_initialValueByClass.push_back(std::make_pair(value, initValue)); continue;}
        if (key == Marker_InitRace) {input >> value; input >> initValue; m_initialValueByRace.push_back(std::make_pair(value, initValue)); continue;}
        if (key == Marker_InitAltar) {input >> value; input >> initValue; m_initialValueByAltar.push_back(std::make_pair(value, initValue)); continue;}
        if (key == Marker_InitGender) {input >> value; input >> initValue; m_initialValueByGender.push_back(std::make_pair(value, initValue)); continue;}
    }

    return false;
}

void Faction::Write(std::ostream & output) const
{
    output << Marker_Name << ' ' << m_name << '\n';
    output << Marker_FriendThreshold << ' ' << m_friendThreshold << '\n';
    output << Marker_EnemyThreshold << ' ' << m_enemyThreshold << '\n';
    output << Marker_Flags << ' ' << m_flags << '\n';
    
    for (size_t i(0); i < m_allies.size(); ++i) output << Marker_Ally << ' ' << m_allies[i] << '\n';
    for (size_t i(0); i < m_enemies.size(); ++i) output << Marker_Enemy << ' ' << m_enemies[i] << '\n';

    for (size_t i(0); i < m_initialValueByClass.size(); ++i)
        output << Marker_InitClass << ' ' << m_initialValueByClass[i].first << ' ' << m_initialValueByClass[i].second << '\n';

    for (size_t i(0); i < m_initialValueByRace.size(); ++i)
        output << Marker_InitRace << ' ' << m_initialValueByRace[i].first << ' ' << m_initialValueByRace[i].second << '\n';

    for (size_t i(0); i < m_initialValueByAltar.size(); ++i)
        output << Marker_InitAltar << ' ' << m_initialValueByAltar[i].first << ' ' << m_initialValueByAltar[i].second << '\n';

    for (size_t i(0); i < m_initialValueByGender.size(); ++i)
        output << Marker_InitGender << ' ' << m_initialValueByGender[i].first << ' ' << m_initialValueByGender[i].second << '\n';

    output << Marker_End << '\n';
}

int Faction::DetermineStandingModifier(const std::vector<std::pair<unsigned int, int> > & listing, unsigned int value)
{
    int result(0);
    for (size_t i(0); i < listing.size(); ++i)
    {
        if (listing[i].first == value)
            result += listing[i].second;
    }

    return result;
}

int Faction::InitialStanding(const CHAR_DATA & ch) const
{
    int result(0);
    result += DetermineStandingModifier(m_initialValueByClass, ch.class_num);
    result += DetermineStandingModifier(m_initialValueByRace, ch.race);
    result += DetermineStandingModifier(m_initialValueByAltar, ch.recall_to->vnum);
    result += DetermineStandingModifier(m_initialValueByGender, ch.sex);
    return result;
}

bool Faction::ToggleFaction(std::vector<unsigned int> & listing, unsigned int value)
{
    for (size_t i(0); i < listing.size(); ++i)
    {
        if (listing[i] == value)
        {
            listing[i] = listing[listing.size() - 1];
            listing.pop_back();
            return false;
        }
    }

    listing.push_back(value);
    return true;
}

void Faction::SetInit(std::vector<std::pair<unsigned int, int> > & listing, unsigned int key, int value)
{
    // Check for already exists
    for (size_t i(0); i < listing.size(); ++i)
    {
        if (listing[i].first == key)
        {
            if (value == 0)
            {
                listing[i] = listing[listing.size() - 1];
                listing.pop_back();
                return;
            }

            listing[i].second = value;
            return;
        }
    }

    // Does not exist, so add it if meaningful
    if (value != 0)
        listing.push_back(std::make_pair(key, value));
}

const char * Faction::NameFor(Flag flag)
{
    switch (flag)
    {
        case AggroEnemy:    return "aggro_enemy";
        case Fake:          return "fake";
        case DecayRating:   return "decay";
        case NoObscure:     return "no_obscure";
        case MaxFlag: break;
    }

    bug("Unknown faction flag specified to Faction::NameFor", 0);
    return "unknown";
}

Faction::Flag Faction::FlagFor(const char * name)
{
    for (unsigned int i(0); i < MaxFlag; ++i)
    {
        Flag flag(static_cast<Flag>(i));
        if (!str_prefix(name, NameFor(flag)))
            return flag;
    }

    return MaxFlag;
}

std::string FactionStanding::Serialize() const
{
    std::ostringstream output;
    for (size_t i(0); i < m_standings.size(); ++i)
        output << m_standings[i] << ' ';
    
    return output.str();
}

void FactionStanding::Deserialize(const std::string & value)
{
    int standing;
    m_standings.clear();
    std::istringstream input(value);
    if (input.eof())
        return;

    while (true)
    {
        input >> standing;
        m_standings.push_back(standing);
        if (input.eof())
            return;

        if (input.fail())
        {
            bug("Failed to read faction standings", 0);
            return;
        }
    }
}

void FactionStanding::EnsureStandings(CHAR_DATA & ch, unsigned int factionNumber)
{
    const FactionTable & table(FactionTable::Instance());
    if (factionNumber >= table.Count())
    {
        bug("FactionStanding::EnsureStandings called with invalid faction number", 0);
        return;
    }

    // Ensure standing is padded out to the faction
    for (size_t i(m_standings.size()); i <= factionNumber; ++i)
        m_standings.push_back(table[i].InitialStanding(ch));
}

void FactionStanding::Change(CHAR_DATA & ch, unsigned int factionNumber, int amount, int allyAmount, int enemyAmount, bool display, bool changeAll)
{
    if (factionNumber == Faction::None)
        return;

    // Verify PC
    if (IS_NPC(&ch))
    {
        bug("FactionStanding::Change called on NPC", 0);
        return;
    }

    // Ensure standing is padded out to the faction, then make sure this isn't a fake faction
    EnsureStandings(ch, factionNumber);
    const Faction & faction(FactionTable::Instance()[factionNumber]);
    if (faction.HasFlag(Faction::Fake))
        return;

    // Handle the change
    int & standing(m_standings[factionNumber]);
    standing += amount;
    standing = URANGE(Minimum, standing, Maximum);

    // Handle any requisite display
    if (amount != 0 && display)
    {
        std::ostringstream mess;
        if (amount < 0)
        {
            if (standing == Minimum) mess << "Your standing with " << faction.Name() << " cannot get any worse!\n";
            else                     mess << "Your standing with " << faction.Name() << " worsens!\n";
        }
        else
        {
            if (standing == Maximum) mess << "Your standing with " << faction.Name() << " is as good as it can be!\n";
            else                     mess << "Your standing with " << faction.Name() << " improves!\n";
        }

        send_to_char(mess.str().c_str(), &ch);
    }

    // Now handle ally and enemy changes
    if (changeAll)
    {
        for (size_t i(0); i < faction.AllyCount(); ++i) 
            Change(ch, faction.Ally(i), allyAmount, 0, 0, display, false);

        for (size_t i(0); i < faction.EnemyCount(); ++i) 
            Change(ch, faction.Enemy(i), enemyAmount, 0, 0, display, false);
    }
}

int FactionStanding::Value(CHAR_DATA & ch, unsigned int factionNumber)
{
    if (factionNumber == Faction::None)
        return 0;

    EnsureStandings(ch, factionNumber);
    return m_standings[factionNumber];
}

void FactionStanding::Set(CHAR_DATA & ch, unsigned int factionNumber, int value)
{
    EnsureStandings(ch, factionNumber);
    m_standings[factionNumber] = URANGE(Minimum, value, Maximum);
}

void FactionStanding::Decay(CHAR_DATA & ch)
{
    for (size_t i(0); i < FactionTable::Instance().Count(); ++i)
    {
        const Faction & faction(FactionTable::Instance()[i]);
        if (!faction.HasFlag(Faction::DecayRating))
            continue;

        EnsureStandings(ch, i);
        int defaultValue(faction.InitialStanding(ch));
        if (m_standings[i] > defaultValue)
            m_standings[i] -= ((m_standings[i] - defaultValue) / 10);
        else if (m_standings[i] < defaultValue)
            m_standings[i] += ((defaultValue - m_standings[i]) / 10);
    }
}

FactionTable & FactionTable::Instance()
{
    static FactionTable factions(FACTION_FILE);
    return factions;
}

void FactionTable::WriteInstance()
{
    Instance().Write(FACTION_FILE);
}

FactionTable::FactionTable(const char * filename)
{
    // Open the file
    std::ifstream fin(filename);
    if (!fin.is_open())
    {
        bug("Failed to open faction file for read", 0);
        return;
    }

    // Read entries
    while (!fin.eof())
    {
        Faction faction;
        if (!faction.Read(fin))
        {
            bug("Failed to read faction from faction file", 0);
            return;
        }

        m_factions.push_back(faction);
    }
}

void FactionTable::Write(const char * filename) const
{
    // Open the file
    std::ofstream fout(filename);
    if (!fout.is_open())
    {
        bug("Failed to open faction file for write", 0);
        return;
    }

    // Write entries
    for (size_t i(0); i < m_factions.size(); ++i)
        m_factions[i].Write(fout);
}

const Faction * FactionTable::LookupFor(unsigned int factionNumber)
{
    if (factionNumber == Faction::None)
        return NULL;

    if (factionNumber >= Instance().Count())
    {
        bug("Invalid faction number found in FactionTable::LookupFor", 0);
        return NULL;
    }

    return &(Instance()[factionNumber]);
}

const Faction * FactionTable::LookupFor(const CHAR_DATA & ch)
{
    if (!IS_NPC(&ch))
        return NULL;

    return LookupFor(ch.pIndexData->factionNumber);
}

FactionRating FactionTable::CurrentStanding(CHAR_DATA & ch, const CHAR_DATA & mob)
{
    if (!IS_NPC(&mob)) 
        return Rating_None;

    return CurrentStanding(ch, mob.pIndexData->factionNumber);
}

FactionRating FactionTable::CurrentStanding(CHAR_DATA & ch, unsigned int factionNumber)
{
    if (IS_NPC(&ch))
        return Rating_None;

    const Faction * faction(LookupFor(factionNumber));
    if (faction == NULL)
        return Rating_None;

    int standing(ch.pcdata->faction_standing->Value(ch, factionNumber));
    if (standing >= faction->FriendThreshold()) return Rating_Friend;
    if (standing <= faction->EnemyThreshold()) return Rating_Enemy;
    return Rating_None;
}


size_t FactionTable::LookupIndex(const Faction & faction) const
{
    for (size_t i(0); i < m_factions.size(); ++i)
    {
        if (&faction == &m_factions[i])
            return i;
    }

    return Faction::None;
}
