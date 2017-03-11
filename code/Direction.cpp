#include "Direction.h"

const char * Direction::NameFor(Value direction)
{
    switch (direction)
    {
        case North: return "north";
        case East:  return "east";
        case South: return "south";
        case West:  return "west";
        case Up:    return "up";
        case Down:  return "down";
        case Max:   break;
    }

    bug("Invalid direction specified in name call", 0);
    return "?";
}

const char * Direction::DirectionalNameFor(Value direction)
{
    switch (direction)
    {
        case North: return "to the north";
        case East:  return "to the east";
        case South: return "to the south";
        case West:  return "to the west";
        case Up:    return "above you";
        case Down:  return "below you";
        case Max:   break;
    }

    bug("Invalid direction specified in directional name call", 0);
    return "?";
}

const char * Direction::SourceNameFor(Value direction)
{
    switch (direction)
    {
        case North: return "from the north";
        case East:  return "from the east";
        case South: return "from the south";
        case West:  return "from the west";
        case Up:    return "from above";
        case Down:  return "from below";
        case Max:   break;
    }

    bug("Invalid direction specified in source name call", 0);
    return "?";
}

Direction::Value Direction::ValueFor(const char * name)
{
    for (unsigned int i(0); i < Max; ++i)
    {
        if (!str_prefix(name, NameFor(static_cast<Value>(i))))
            return static_cast<Value>(i);
    }

    return Max;
}

Direction::Value Direction::ReverseOf(Value direction)
{
    switch (direction)
    {
        case North: return South;
        case East:  return West;
        case South: return North;
        case West:  return East;
        case Up:    return Down;
        case Down:  return Up;
        case Max:   break;
    }

    bug("Invalid direction specified in reverse call", 0);
    return North;
}

ROOM_INDEX_DATA * Direction::Adjacent(const ROOM_INDEX_DATA & current, Value direction, int stoppedByFlags)
{
    if (current.exit[direction] == NULL) 
        return NULL;

    if (IS_SET(current.exit[direction]->exit_info, stoppedByFlags))
        return NULL;

    return current.exit[direction]->u1.to_room;
}

std::vector<Direction::Value> Direction::ValidDirectionsFrom(const ROOM_INDEX_DATA & room, Value ignoreDirection, int stoppedByFlags)
{
    std::vector<Direction::Value> result;
    for (unsigned int i(0); i < Max; ++i)
    {
        Value value(static_cast<Value>(i));
        if (value != ignoreDirection && Adjacent(room, value, stoppedByFlags) != NULL)
            result.push_back(value);
    }
    return result;
}
