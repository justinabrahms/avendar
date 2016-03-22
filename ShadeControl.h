#ifndef SHADECONTROL_H
#define SHADECONTROL_H

#include "merc.h"
#include <vector>

class ShadeControl
{
    public:
        static bool CheckShadeTimer(CHAR_DATA & ch);
        static bool CheckShadeAttacked(CHAR_DATA & ch, CHAR_DATA & victim);
        static void Update();
        static void UpdateShadePathing();
        static std::vector<const ROOM_INDEX_DATA *> SpectralLanternRooms();

    private:
        static unsigned int CountShadesAndRooms(const AREA_DATA & area, std::vector<ROOM_INDEX_DATA*> & rooms);
        static void UpdateArea(const AREA_DATA & area, unsigned int shadeCount, std::vector<ROOM_INDEX_DATA*> & rooms, const std::vector<const ROOM_INDEX_DATA*> & lanterns);
        static bool UpdateRoom(ROOM_INDEX_DATA & room, const std::vector<const ROOM_INDEX_DATA*> & lanterns);
        static void ShowShadeMessage(const CHAR_DATA & shade, const char * message);
        static void UpdateShadePath(CHAR_DATA & ch, const std::vector<const ROOM_INDEX_DATA*> & rooms);
        static void DescribeShade(CHAR_DATA & shade);
};

#endif
