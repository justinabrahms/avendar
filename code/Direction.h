#ifndef DIRECTION_H
#define DIRECTION_H

#include "merc.h"
#include <vector>

class Direction
{
    public:
        enum Value {North = 0, East = 1, South = 2, West = 3, Up = 4, Down = 5, Max = 6};

        static Value ReverseOf(Value direction);
        static const char * NameFor(Value direction);
        static const char * DirectionalNameFor(Value direction);
        static const char * SourceNameFor(Value direction);
        static Value ValueFor(const char * name);
        static char LetterFor(Value direction);
        static ROOM_INDEX_DATA * Adjacent(const ROOM_INDEX_DATA & current, Value direction, int stoppedByFlags = 0);
        static std::vector<Value> ValidDirectionsFrom(const ROOM_INDEX_DATA & room, Value ignoreDirection = Max, int stoppedByFlags = 0);
};

inline char Direction::LetterFor(Direction::Value direction) {return NameFor(direction)[0];}

#endif
