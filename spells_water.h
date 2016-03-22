#ifndef SPELLS_WATER_H
#define SPELLS_WATER_H

#include "merc.h"
#include <string>
#include <set>

std::set<ROOM_INDEX_DATA*> connected_water_rooms(ROOM_INDEX_DATA & startRoom);
std::string checkDiagnoseTags(CHAR_DATA * ch, CHAR_DATA * victim);
bool check_group_fieldmedicine_save(int sn, int diseaseLevel, CHAR_DATA * ch);
bool is_water_room(const ROOM_INDEX_DATA & room);
CHAR_DATA * make_water_elemental(int level);
bool check_encasecasting(int level, CHAR_DATA * ch, CHAR_DATA * victim);

#endif
