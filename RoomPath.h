#ifndef ROOMPATH_H
#define ROOMPATH_H

#include "merc.h"
#include <vector>

class RoomPathTracker;

class RoomPath
{
    friend class RoomPathTracker;

    public:
        static const unsigned int Infinite = static_cast<unsigned int>(-1);

        RoomPath(const ROOM_INDEX_DATA & start, const ROOM_INDEX_DATA & end, CHAR_DATA * actor = NULL, unsigned int maxSteps = Infinite, int stoppedByFlags = 0);

        bool Exists() const;
        unsigned int StepCount() const;
        unsigned int StepDirection(unsigned int index) const;

    private:
        /// Implementation
        void HandlePathFound(const ROOM_INDEX_DATA & start, const ROOM_INDEX_DATA & end, unsigned int radius);
        static void IncrementID();

        /// Members
        bool m_exists;
        std::vector<unsigned int> m_path;

        /// Static members
        static unsigned int s_generationID;
};

inline bool RoomPath::Exists() const {return m_exists;}
inline unsigned int RoomPath::StepCount() const {return m_path.size();}
inline unsigned int RoomPath::StepDirection(unsigned int index) const {return m_path[index];}

class RoomPathTracker
{
    public:
        explicit RoomPathTracker(const RoomPath & roomPath);

        bool HasStep() const;
        unsigned int PeekStep() const;
        unsigned int StepsRemaining() const;
        unsigned int Step();

    private:
        unsigned int m_index;
        std::vector<unsigned int> m_path;
};

inline RoomPathTracker::RoomPathTracker(const RoomPath & roomPath) : m_index(0), m_path(roomPath.m_path) {}
inline bool RoomPathTracker::HasStep() const {return (m_index < m_path.size());}
inline unsigned int RoomPathTracker::PeekStep() const {return m_path[m_index];}
inline unsigned int RoomPathTracker::StepsRemaining() const {return (m_path.size() - m_index);}
inline unsigned int RoomPathTracker::Step() {return m_path[m_index++];}

void ClearPath(CHAR_DATA * ch);
void AssignNewPath(CHAR_DATA * ch, const RoomPath & roomPath);
bool StepAlongPath(CHAR_DATA * ch);

#endif
