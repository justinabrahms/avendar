#ifndef LEYGROUP_H
#define LEYGROUP_H

#include <cstddef>
#include <vector>

class LeyLine;

class Fount
{
    public:
        // If you edit this enum make sure to edit BuildInfos() as well!
        enum Frequency {Always, Often, Occasionally, Rarely, Never, Default, Max};

        static const int PowerMax = 2;
        static const int PowerMin = -2;

        static Frequency ValueFor(const char * name);
        static const char * NameFor(Frequency frequency);
        static unsigned int SkipChance(Frequency roomFrequency);
        static unsigned int FountsPer100Rooms(Frequency areaFrequency);

    private:
        /// Types
        struct Info
        {
            Info(Frequency frequencyIn, const char * nameIn, unsigned int skipChanceIn, unsigned int areaFountsPer100In);

            Frequency frequency;
            const char * name;
            unsigned int skipChance;
            unsigned int areaFountsPer100;
        };

        /// Implementation
        static const std::vector<Info> & BuildInfos();
        static const std::vector<Info> & Infos();
};

class LeyInfo
{
    public:
        LeyInfo(void * id, int orderPower, int positivePower);

        void * ID;
        int OrderPower;
        int PositivePower;
};

class LeyGroup
{
    public:
        static const unsigned int NotFound = static_cast<unsigned int>(-1);
        static const long Unattuned = -1;

        LeyGroup(const LeyInfo & fount = LeyInfo(NULL, 0, 0));
        void SetFount(const LeyInfo & fount);
        void ClearFount();
        void Add(const LeyInfo & leyInfo);
        void Remove(unsigned int index);
        int TotalOrderPower() const;
        int TotalPositivePower() const;
        int FountPositivePower() const;
        int FountOrderPower() const;
        bool HasFount() const;
        unsigned int LineCount() const;
        const LeyInfo & Line(unsigned int index) const;
        unsigned int LineByID(const void * id) const;
        long AttunedID() const;
        void SetAttunedID(long id);

    private:
        long m_attunedID;
        LeyInfo m_fount;
        std::vector<LeyInfo> m_infos;
};

inline Fount::Info::Info(Frequency frequencyIn, const char * nameIn, unsigned int skipChanceIn, unsigned int areaFountsPer100In) : 
    frequency(frequencyIn), name(nameIn), skipChance(skipChanceIn), areaFountsPer100(areaFountsPer100In) {}
inline const char * Fount::NameFor(Frequency frequency) {return Infos()[frequency].name;}
inline unsigned int Fount::SkipChance(Frequency frequency) {return Infos()[frequency].skipChance;}
inline unsigned int Fount::FountsPer100Rooms(Frequency frequency) {return Infos()[frequency].areaFountsPer100;}

inline LeyInfo::LeyInfo(void * id, int orderPower, int positivePower) : ID(id), OrderPower(orderPower), PositivePower(positivePower) {}

inline LeyGroup::LeyGroup(const LeyInfo & fount) : m_attunedID(Unattuned), m_fount(fount) {}
inline void LeyGroup::SetFount(const LeyInfo & fount) {m_fount = fount;}
inline void LeyGroup::ClearFount() {SetFount(LeyInfo(NULL, 0, 0));}
inline void LeyGroup::Add(const LeyInfo & leyInfo) {m_infos.push_back(leyInfo);}
inline bool LeyGroup::HasFount() const {return (m_fount.ID != NULL);}
inline int LeyGroup::FountPositivePower() const {return m_fount.PositivePower;}
inline int LeyGroup::FountOrderPower() const {return m_fount.OrderPower;}
inline unsigned int LeyGroup::LineCount() const {return m_infos.size();}
inline const LeyInfo & LeyGroup::Line(unsigned int index) const {return m_infos[index];}
inline long LeyGroup::AttunedID() const {return (HasFount() ? m_attunedID : Unattuned);}
inline void LeyGroup::SetAttunedID(long id) {m_attunedID = id;}

#endif
