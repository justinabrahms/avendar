#ifndef SPELLS_EARTH_H
#define SPELLS_EARTH_H

#include "merc.h"
#include "Direction.h"

void performAdamantineCast(CHAR_DATA * ch, CHAR_DATA * victim, int sn, int level, const char * currTargetName);
void shake_ground(CHAR_DATA * ch, ROOM_INDEX_DATA & room, int sn, int dam, bool ignoreGroup);
void check_bedrockroots(CHAR_DATA & ch);
int determine_saltoftheearth_level(CHAR_DATA & ch, int sphere);
void check_glyphofentombment(ROOM_INDEX_DATA & room, Direction::Value direction);
void check_tuningstone(CHAR_DATA & ch, ROOM_INDEX_DATA & room);
void check_mudfootcurse(CHAR_DATA & ch);
bool is_stabilized(const CHAR_DATA & ch);

#endif
