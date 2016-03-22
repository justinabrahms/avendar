#include "RoomPath.h"
#include "Direction.h"
#include <algorithm>

unsigned int RoomPath::s_generationID(0);

RoomPath::RoomPath(const ROOM_INDEX_DATA & start, const ROOM_INDEX_DATA & end, CHAR_DATA * actor, unsigned int maxSteps, int stoppedByFlags) :
    m_exists(false)
{
    // Increment the generation ID to effectively clear any previously-generated pathing for the rooms in question
    // This won't obviate created RoomPaths, just allow the invasive variables within the ROOM_INDEX_DATAs to be
    // repurposed for this one; we don't want previous logic tripping us up. This step would not be necessary
    // if we made the algoritm non-invasive, but then our runtime would rise based on the data structure used
    IncrementID();

    // Seed the current radius and working set of rooms
    unsigned int radius(0);
    std::vector<const ROOM_INDEX_DATA*> roomBuffer0;
    std::vector<const ROOM_INDEX_DATA*> roomBuffer1;
    std::vector<const ROOM_INDEX_DATA*> * rooms(&roomBuffer0);
    std::vector<const ROOM_INDEX_DATA*> * nextRooms(&roomBuffer1);
    rooms->push_back(&start);

    // Expand in a growing 'circle' until the room is found, the max is reached, or no more rooms are available to traverse
    while ((maxSteps == Infinite || radius <= maxSteps) && !rooms->empty())
    {
        // Operate on the current working set of rooms
        for (size_t i(0); i < rooms->size(); ++i)
        {
            const ROOM_INDEX_DATA & currRoom(*((*rooms)[i]));
            if (&currRoom == &end)
            {
                // We've reached our goal; build the result and return
                HandlePathFound(start, end, radius);
                return;
            }

            // Not yet at the end, so iterate the exits
            for (unsigned int door(0); door < MAX_DOOR; ++door)
            {
                // Get the room past this exit, if it exists and has not already been processed
                const ROOM_INDEX_DATA * nextRoom(Direction::Adjacent(currRoom, static_cast<Direction::Value>(door), stoppedByFlags));
                if (nextRoom == NULL || nextRoom->path_generation_id >= s_generationID 
                || (actor != NULL && !can_see_room(actor, const_cast<ROOM_INDEX_DATA*>(nextRoom))))
                    continue;

                // Room is valid, point it back to this room and add it to the next working set
                nextRoom->arrived_from = &currRoom;
                nextRoom->arrived_from_door = door;
                nextRoom->path_generation_id = s_generationID;
                nextRooms->push_back(nextRoom);
            }
        }

        // All rooms from the working set processed, so swap the buffers to process the next working set
        std::swap(rooms, nextRooms);
        nextRooms->clear();
        ++radius;
    }
}

void RoomPath::HandlePathFound(const ROOM_INDEX_DATA & start, const ROOM_INDEX_DATA & end, unsigned int radius)
{
    m_exists = true;
    m_path.resize(radius);

    // Work backwards to recover the path
    unsigned int i(radius - 1);
    for (const ROOM_INDEX_DATA * curr(&end); curr != &start; curr = curr->arrived_from)
    {
        // The prereq to this function guarantees the correctness of radius, so i should be sane here
        m_path[i] = curr->arrived_from_door;
        --i;
    }
}

void RoomPath::IncrementID()
{
    // Increment and check for overflow
    ++s_generationID;
    if (s_generationID != 0)
        return;

    // Handle overflow by incrementing once more and clearing all room IDs
    s_generationID = 1;
    for (unsigned int iHash(0); iHash < MAX_KEY_HASH; ++iHash)
    {
        for (ROOM_INDEX_DATA * curr(room_index_hash[iHash]); curr != NULL; curr = curr->next)
            curr->path_generation_id = 0;
    }
}

void ClearPath(CHAR_DATA * ch)
{
    delete ch->path;
    ch->path = NULL;
}

void AssignNewPath(CHAR_DATA * ch, const RoomPath & roomPath)
{
    ClearPath(ch);
    ch->path = new RoomPathTracker(roomPath);
}

bool StepAlongPath(CHAR_DATA * ch)
{
    // Check for a path
    if (ch->path == NULL)
        return false;

    // Check for more steps in the path
    if (!ch->path->HasStep())
    {
        ClearPath(ch);
        return false;
    }

    // Attempt to perform the next step
    unsigned int door(ch->path->Step());
    ROOM_INDEX_DATA * startRoom(ch->in_room);
    if (startRoom == NULL || startRoom->exit[door] == NULL)
    {
        ClearPath(ch);
        return false;
    }

    // Make sure the step happened
    move_char(ch, door, true);
    if (startRoom == ch->in_room)
    {
        ClearPath(ch);
        return false;
    }

    return true;
}
