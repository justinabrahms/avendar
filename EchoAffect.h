#ifndef ECHOAFFECT_H
#define ECHOAFFECT_H

#include <list>
#include <string>
#include "merc.h"

class EchoAffect
{
    public:
        static const int GSN = -1000;
        typedef bool (*Callback)(CHAR_DATA * ch, EchoAffect * thisAffect, void * tag);
        typedef bool (*MoveCallback)(CHAR_DATA * ch, ROOM_INDEX_DATA * room, EchoAffect * thisAffect, void * tag);
        typedef bool (*PositionCallback)(CHAR_DATA * ch, int newPos, EchoAffect * thisAffect, void * tag);
        typedef bool (*CastCallback)(CHAR_DATA * ch, int sn, int level, void * vo, int target, EchoAffect * thisAffect, void * tag);

        explicit EchoAffect(unsigned int pulseDelay = 1);
        void AddLine(Callback extraAction, const std::string & toChar, OBJ_DATA * obj = NULL, CHAR_DATA * victim = NULL);
        void AddLine(Callback extraAction, const std::string & toChar, const std::string & toRoom, OBJ_DATA * obj = NULL, CHAR_DATA * victim = NULL);
        void AddLine(Callback extraAction, const std::string & toChar, const std::string & toVict, const std::string & toNotVict, OBJ_DATA * obj = NULL, CHAR_DATA * victim = NULL);
        
        void SetMoveCallback(MoveCallback callback);
        void SetPositionCallback(PositionCallback callback);
        void SetCastCallback(CastCallback callback);
        void SetTag(void * tag);
        
        void * Tag() const;
        unsigned int LinesRemaining() const;
        
        bool HandleNext(CHAR_DATA * ch);
        bool HandleMoveFrom(CHAR_DATA * ch, ROOM_INDEX_DATA * room);
        bool HandlePositionChange(CHAR_DATA * ch, int newPos);
        bool HandleCastSpell(CHAR_DATA * ch, int sn, int level, void * vo, int target);

        static void ApplyToChar(CHAR_DATA * ch, EchoAffect * echoAffect);

    private:
        struct LineInfo
        {
            LineInfo(Callback extraAction, const std::string & toChar, const std::string & toVict, const std::string & toNotVict, OBJ_DATA * obj, CHAR_DATA * victim);

            Callback ExtraAction;
            std::string ToChar;
            std::string ToVict;
            std::string ToNotVict;
            ROOM_INDEX_DATA * Room;
            CHAR_DATA * Victim;
            OBJ_DATA * Object;
        };


        // Members
        MoveCallback m_moveHandler;
        PositionCallback m_positionHandler;
        CastCallback m_castHandler;
        unsigned int m_pulseDelay;
        unsigned int m_pulsesUntilNext;
        void * m_tag;
        std::list<LineInfo> m_lines;
};

inline EchoAffect::EchoAffect(unsigned int pulseDelay) : m_moveHandler(NULL), m_positionHandler(NULL), m_castHandler(NULL), m_pulseDelay(pulseDelay), m_pulsesUntilNext(pulseDelay), m_tag(NULL) {}
inline void EchoAffect::AddLine(Callback extraAction, const std::string & toChar, OBJ_DATA * obj, CHAR_DATA * victim) {AddLine(extraAction, toChar, "", "", obj, victim);}
inline void EchoAffect::AddLine(Callback extraAction, const std::string & toChar, const std::string & toRoom, OBJ_DATA * obj, CHAR_DATA * victim) {AddLine(extraAction, toChar, toRoom, "", obj, victim);}
inline void EchoAffect::AddLine(Callback extraAction, const std::string & toChar, const std::string & toVict, const std::string & toNotVict, OBJ_DATA * obj, CHAR_DATA * victim) 
{m_lines.push_back(LineInfo(extraAction, toChar, toVict, toNotVict, obj, victim));}
inline void EchoAffect::SetMoveCallback(MoveCallback callback) {m_moveHandler = callback;}
inline void EchoAffect::SetPositionCallback(PositionCallback callback) {m_positionHandler = callback;}
inline void EchoAffect::SetCastCallback(CastCallback callback) {m_castHandler = callback;}
inline void EchoAffect::SetTag(void * tag) {m_tag = tag;}
inline void * EchoAffect::Tag() const {return m_tag;}
inline unsigned int EchoAffect::LinesRemaining() const {return m_lines.size();}

#endif
