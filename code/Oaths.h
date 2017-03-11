#ifndef OATHS_H
#define OATHS_H

#include "merc.h"
#include "DisplayPanel.h"
#include <vector>
#include <string>
#include <iostream>

class Oaths
{
    /// Constants
    static const time_t Lifetime = -1;
    static const int MinimumDuration = 4;
    
    /// Types
    class CharInfo
    {
        public:
            explicit CharInfo(const CHAR_DATA & ch);
            
            CHAR_DATA * lookup() const;
            bool isMatch(const CHAR_DATA & ch) const; 
            const std::string & name() const;
            
            void serialize(std::ostream & out) const;
            static CharInfo deserialize(std::istream & in);

        private:
            CharInfo();
            explicit CharInfo(std::istream & in);

            // Members
            long m_id;
            std::string m_name;
    };

    class Oath
    {
        public:
            enum State {Normal = 0, Broken, Released, Max};

            Oath(const CHAR_DATA & oathGiver, const CHAR_DATA & oathHolder, time_t expiration);

            State state() const;
            bool isExpired() const;
            bool isGiver(const std::string & giverName) const;
            bool isGiver(const CHAR_DATA & ch) const;
            bool isHolder(const CHAR_DATA & ch) const;
            CHAR_DATA * lookupGiver() const;
            CHAR_DATA * lookupHolder() const;
            const std::string & giverName() const;
            const std::string & holderName() const;
            int daysLeft() const;

            void setDaysLeft(int daysLeft);
            void setState(State state);
            void shrinkDurationToMinimum();

            void serialize(std::ostream & out) const;
            static Oath deserialize(std::istream & in);
           
        private:
            Oath();
            explicit Oath(std::istream & in);

            // Members
            time_t m_expiration;
            State m_state;
            CharInfo m_oathGiver;
            CharInfo m_oathHolder;
    };

    class OathLister
    {
        public:
            OathLister();
            void add(const Oath & oath);
            std::string render() const;

        private:
            DisplayPanel::Box m_stateBox;
            DisplayPanel::Box m_giverBox;
            DisplayPanel::Box m_holderBox;
            DisplayPanel::Box m_daysBox;
    };

    public:
        static bool LoadOaths();
        static void UpdateOaths();
        static bool IsOathBreaker(const CHAR_DATA & ch);
        static CHAR_DATA * OathHolderFor(const CHAR_DATA & ch);
        static void DoOath(CHAR_DATA & ch, const char * argument);

    private:
        static void ListSyntax(CHAR_DATA & ch);
        static void ListSetSyntax(CHAR_DATA & ch);
        static void ListAllOaths(CHAR_DATA & ch);
        static void ListOaths(CHAR_DATA & ch, const std::vector<std::string> & arguments);
        static void SetOath(CHAR_DATA & ch, const std::vector<std::string> & arguments);
        static void GiveOath(CHAR_DATA & ch, const std::vector<std::string> & arguments);
        static void ReleaseOath(CHAR_DATA & ch, const std::vector<std::string> & arguments);
        static void BrokenOath(CHAR_DATA & ch, const std::vector<std::string> & arguments);

        static void SetOathState(CHAR_DATA & ch, Oath & oath, size_t index, const std::string & value);
        static void SetOathDays(CHAR_DATA & ch, Oath & oath, const std::string & value);

        template <typename Type> static Oath * OathFrom(const Type & ch);
        template <typename Type> static Oath * OathFrom(const Type & ch, size_t & index);
        static std::vector<Oath*> OathsTo(const CHAR_DATA & ch);
        static void SaveOaths();
        static void RemoveOath(size_t index);

        /// Data
        static std::vector<Oath> s_oaths; 
};

#endif
