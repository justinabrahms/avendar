#include "EchoAffect.h"
#include <sstream>

EchoAffect::LineInfo::LineInfo(Callback extraAction, const std::string & toChar, const std::string & toVict, const std::string & toNotVict, OBJ_DATA * obj, CHAR_DATA * victim) :
    ExtraAction(extraAction),
    ToChar(toChar),
    ToVict(toVict),
    ToNotVict(toNotVict),
    Victim(victim),
    Object(obj)
{}

bool EchoAffect::HandleMoveFrom(CHAR_DATA * ch, ROOM_INDEX_DATA * room)
{
    if (m_moveHandler == NULL)
        return false;

    return (*m_moveHandler)(ch, room, this, m_tag);
}

bool EchoAffect::HandlePositionChange(CHAR_DATA * ch, int newPos)
{
    if (m_positionHandler == NULL)
        return false;

    return (*m_positionHandler)(ch, newPos, this, m_tag);
}

bool EchoAffect::HandleCastSpell(CHAR_DATA * ch, int sn, int level, void * vo, int target)
{
    if (m_castHandler == NULL)
        return false;

    return (*m_castHandler)(ch, sn, level, vo, target, this, m_tag);
}

bool EchoAffect::HandleNext(CHAR_DATA * ch)
{
    // Handle delay counter
    --m_pulsesUntilNext;
    if (m_pulsesUntilNext > 0)
        return false;

    m_pulsesUntilNext = m_pulseDelay;

    // Handle empty lines
    if (m_lines.empty())
    {
        std::ostringstream mess;
        mess << "Failed to strip (or instantiate lines on) EchoAffect on character " << ch->name;
        bug(mess.str().c_str(), 0);
        return true;
    }

    // Handle a non-empty line; start by checking the room
    if (ch->in_room == NULL)
    {
        std::ostringstream mess;
        mess << "Character " << ch->name << " is not in room for EchoAffect";
        bug(mess.str().c_str(), 0);
        return true;
    }

    // Now check the obj
    const LineInfo & lineInfo(m_lines.front());
    OBJ_DATA * obj(lineInfo.Object);
    if (obj != NULL)
    {
        std::ostringstream mess;
        if (!verify_obj_world(obj))
        {
            mess << "Cannot verify object in world for " << ch->name << "'s EchoAffect";
            bug(mess.str().c_str(), 0);
            return true;
        }

        if (obj->in_room != ch->in_room)
        {
            mess << "Object '" << obj->short_descr << " is not in same room as " << ch->name << " for EchoAffect";
            bug(mess.str().c_str(), 0);
            return true;
        }
    }

    // Now check the victim
    CHAR_DATA * victim(lineInfo.Victim);
    if (victim != NULL && !verify_char_room(victim, ch->in_room))
    {
        std::ostringstream mess;
        mess << "Cannot verify target in same room as " << ch->name << " for EchoAffect";
        bug(mess.str().c_str(), 0);
        return true;
    }

    // Everything checks out
    if (!lineInfo.ToChar.empty())
        act(lineInfo.ToChar.c_str(), ch, obj, victim, TO_CHAR);

    if (victim == NULL || lineInfo.ToNotVict.empty())
    {
        if (!lineInfo.ToVict.empty())
            act(lineInfo.ToVict.c_str(), ch, obj, victim, TO_ROOM);
    }
    else
    {
        act(lineInfo.ToVict.c_str(), ch, obj, victim, TO_VICT);
        act(lineInfo.ToNotVict.c_str(), ch, obj, victim, TO_NOTVICT);
    }

    // Call the extra action callback
    bool result(false);
    if (lineInfo.ExtraAction != NULL)
    {
        if ((*lineInfo.ExtraAction)(ch, this, m_tag))
            result = true;
    }
    
    m_lines.pop_front();
    return (result || m_lines.empty());
}

void EchoAffect::ApplyToChar(CHAR_DATA * ch, EchoAffect * echoAffect)
{
     AFFECT_DATA af = {0};
     af.where    = TO_AFFECTS;
     af.type     = GSN;
     af.level    = ch->level;
     af.duration = -1;
     af.valid    = true;
     af.point    = echoAffect;
     affect_to_char(ch, &af);
}
