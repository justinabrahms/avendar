#ifndef FACTION_H
#define FACTION_H

#include <string>
#include <vector>
#include <bitset>
#include <fstream>
#include "merc.h"

enum FactionRating {Rating_Friend, Rating_Enemy, Rating_None};

class Faction
{
    /// Constants
    static const char * Marker_Name;
    static const char * Marker_FriendThreshold;
    static const char * Marker_EnemyThreshold;
    static const char * Marker_Flags;
    static const char * Marker_Ally;
    static const char * Marker_Enemy;
    static const char * Marker_InitClass;
    static const char * Marker_InitRace;
    static const char * Marker_InitAltar;
    static const char * Marker_InitGender;
    static const char * Marker_End;

    public:
        static const unsigned int None = static_cast<unsigned int>(-1);
        enum Flag {AggroEnemy = 0, Fake, DecayRating, NoObscure, MaxFlag};

        Faction();

        bool Read(std::istream & input);
        void Write(std::ostream & output) const;
        int InitialStanding(const CHAR_DATA & ch) const;

        // Basic Mutators
        void SetName(const char * name);
        void ToggleFlag(Flag flag);
        void SetFriendThreshold(int value);
        void SetEnemyThreshold(int value);
        bool ToggleEnemy(unsigned int value);
        bool ToggleAlly(unsigned int value);
        void SetInitClass(unsigned int key, int value);
        void SetInitRace(unsigned int key, int value);
        void SetInitAltar(unsigned int key, int value);
        void SetInitGender(unsigned int key, int value);

        // Basic Accessors
        std::string Name() const;
        int FriendThreshold() const;
        int EnemyThreshold() const;
        bool HasFlag(Flag flag) const;

        size_t AllyCount() const;
        size_t EnemyCount() const;
        unsigned int Ally(size_t index) const;
        unsigned int Enemy(size_t index) const;

        size_t InitClassCount() const;
        size_t InitRaceCount() const;
        size_t InitAltarCount() const;
        size_t InitGenderCount() const;

        std::pair<unsigned int, int> InitClass(size_t index) const;
        std::pair<unsigned int, int> InitRace(size_t index) const;
        std::pair<unsigned int, int> InitAltar(size_t index) const;
        std::pair<unsigned int, int> InitGender(size_t index) const;

        static const char * NameFor(Flag flag);
        static Flag FlagFor(const char * name);

    private:
        // Methods
        static int DetermineStandingModifier(const std::vector<std::pair<unsigned int, int> > & listing, unsigned int value);
        static bool ToggleFaction(std::vector<unsigned int> & listing, unsigned int value); 
        static void SetInit(std::vector<std::pair<unsigned int, int> > & listing, unsigned int key, int value);

        // Members
        std::string m_name;
        std::vector<unsigned int> m_allies;
        std::vector<unsigned int> m_enemies;
        std::vector<std::pair<unsigned int, int> > m_initialValueByClass;
        std::vector<std::pair<unsigned int, int> > m_initialValueByRace;
        std::vector<std::pair<unsigned int, int> > m_initialValueByAltar;
        std::vector<std::pair<unsigned int, int> > m_initialValueByGender;
        int m_friendThreshold;
        int m_enemyThreshold;
        std::bitset<MaxFlag> m_flags;
};

inline void Faction::SetName(const char * name) {m_name = name;}
inline void Faction::ToggleFlag(Flag flag) {m_flags.flip(flag);}
inline void Faction::SetFriendThreshold(int value) {m_friendThreshold = value;}
inline void Faction::SetEnemyThreshold(int value) {m_enemyThreshold = value;}
inline bool Faction::ToggleEnemy(unsigned int value) {return ToggleFaction(m_enemies, value);}
inline bool Faction::ToggleAlly(unsigned int value) {return ToggleFaction(m_allies, value);}
inline void Faction::SetInitClass(unsigned int key, int value) {SetInit(m_initialValueByClass, key, value);}
inline void Faction::SetInitRace(unsigned int key, int value) {SetInit(m_initialValueByRace, key, value);}
inline void Faction::SetInitAltar(unsigned int key, int value) {SetInit(m_initialValueByAltar, key, value);}
inline void Faction::SetInitGender(unsigned int key, int value) {SetInit(m_initialValueByGender, key, value);}

inline std::string Faction::Name() const {return m_name;}
inline int Faction::FriendThreshold() const {return m_friendThreshold;}
inline int Faction::EnemyThreshold() const {return m_enemyThreshold;}
inline bool Faction::HasFlag(Flag flag) const {return m_flags.test(flag);}
inline size_t Faction::AllyCount() const {return m_allies.size();}
inline size_t Faction::EnemyCount() const {return m_enemies.size();}
inline unsigned int Faction::Ally(size_t index) const {return m_allies[index];}
inline unsigned int Faction::Enemy(size_t index) const {return m_enemies[index];}

inline size_t Faction::InitClassCount() const {return m_initialValueByClass.size();}
inline size_t Faction::InitRaceCount() const {return m_initialValueByRace.size();}
inline size_t Faction::InitAltarCount() const {return m_initialValueByAltar.size();}
inline size_t Faction::InitGenderCount() const {return m_initialValueByGender.size();}

inline std::pair<unsigned int, int> Faction::InitClass(size_t index) const {return m_initialValueByClass[index];}
inline std::pair<unsigned int, int> Faction::InitRace(size_t index) const {return m_initialValueByRace[index];}
inline std::pair<unsigned int, int> Faction::InitAltar(size_t index) const {return m_initialValueByAltar[index];}
inline std::pair<unsigned int, int> Faction::InitGender(size_t index) const {return m_initialValueByGender[index];}

class FactionStanding
{
    public:
        static const int Minimum = -10000;
        static const int Maximum = 10000;
    
        std::string Serialize() const;
        void Deserialize(const std::string & value);
        void Change(CHAR_DATA & ch, unsigned int factionNumber, int amount, int allyAmount, int enemyAmount, bool display, bool changeAll = true);
        int Value(CHAR_DATA & ch, unsigned int factionNumber);
        void Set(CHAR_DATA & ch, unsigned int factionNumber, int value);
        void Decay(CHAR_DATA & ch);

    private:
        // Methods
        void EnsureStandings(CHAR_DATA & ch, unsigned int factionNumber);

        // Members
        std::vector<int> m_standings;
};

class FactionTable
{
    public:
        explicit FactionTable(const char * filename);

        size_t Count() const;
        void Write(const char * filename) const;

        void Add();
        size_t LookupIndex(const Faction & faction) const;

        Faction & operator[](size_t index);
        const Faction & operator[](size_t index) const;

        // Static interface
        static FactionTable & Instance();
        static void WriteInstance();
        static const Faction * LookupFor(const CHAR_DATA & ch);
        static const Faction * LookupFor(unsigned int factionNumber);
        static FactionRating CurrentStanding(CHAR_DATA & ch, const CHAR_DATA & mob);
        static FactionRating CurrentStanding(CHAR_DATA & ch, unsigned int factionNumber);

    private:
        std::vector<Faction> m_factions;
};

inline size_t FactionTable::Count() const {return m_factions.size();}
inline void FactionTable::Add() {m_factions.push_back(Faction());}
inline Faction & FactionTable::operator[](size_t index) {return m_factions[index];}
inline const Faction & FactionTable::operator[](size_t index) const {return m_factions[index];}

#endif
