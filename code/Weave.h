#ifndef WEAVE_H
#define WEAVE_H

#include "merc.h"
#include "Direction.h"
#include <vector>
#include <set>
#include <string>

class Weave
{
    static const unsigned int MinLeyLength = 3;
    static const unsigned int AverageLeyLength = 12;

    public:
        static void SaveWeave();
        static bool LoadWeave();
        static void Regenerate();
        static void ShowWeave(CHAR_DATA & ch, const ROOM_INDEX_DATA & room);
        static bool HasWeave(const ROOM_INDEX_DATA & room);
        static int AbsolutePositivePower(const ROOM_INDEX_DATA & room);
        static int AbsoluteOrderPower(const ROOM_INDEX_DATA & room);
        static void UpdateAttunements();
    
    private:
        static std::set<ROOM_INDEX_DATA *> s_changedRooms;

        static void UpdateAttunementsFor(CHAR_DATA & ch, int count);
        static void Regenerate(AREA_DATA & area);
        static void RemoveFount(ROOM_INDEX_DATA & room);
        static void RerouteFount(ROOM_INDEX_DATA & room);
        static void CreateFount(ROOM_INDEX_DATA & room);
        static void ClearLeyLines(ROOM_INDEX_DATA & room);
        static void GenerateLeyLines(ROOM_INDEX_DATA & room);
        static Direction::Value PopNextDirection(const ROOM_INDEX_DATA & room, std::vector<Direction::Value> & directions);
        static int CalculatePower(int bias);
        static int CalculateOrderPower(const ROOM_INDEX_DATA & room);
        static int CalculatePositivePower(const ROOM_INDEX_DATA & room);
        static void ContinueLeyLine(ROOM_INDEX_DATA & room, Direction::Value direction, 
                                    void * id, unsigned int length, 
                                    unsigned int expectedLength, int orderPower, int positivePower);

        static void ShowNew(const ROOM_INDEX_DATA & room);
        static std::vector<Direction::Value> AdjacentLeyRooms(const ROOM_INDEX_DATA & room, const void * id);
        static std::string GenerateFountDescription(int orderPower, int positivePower, const std::vector<Direction::Value> & directions);
        static std::string GenerateLineDescription(int orderPower, int positivePower, const std::vector<Direction::Value> & directions);
        static std::string DirectionString(Direction::Value value, bool raw);

        static int ApplyPositiveModifiers(int positivePower, const ROOM_INDEX_DATA & room);
        static int ApplyOrderModifiers(int orderPower, const ROOM_INDEX_DATA & room);
};

#endif
