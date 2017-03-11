#ifndef SPELLSAIR_H
#define SPELLSAIR_H

#include "merc.h"
#include "Direction.h"
#include <utility>

std::pair<Direction::Value, int> checkWind(CHAR_DATA * ch);
void check_displacement(CHAR_DATA * ch);
void check_clingingfog(CHAR_DATA * ch);
void check_conduitoftheskies(CHAR_DATA * ch);
void finishLearningPhantasm(CHAR_DATA * ch, AFFECT_DATA * paf);
AFFECT_DATA * get_charging_effect(CHAR_DATA * ch);
AFFECT_DATA * get_charged_effect(CHAR_DATA * ch);
std::string generate_unreal_predicate();
std::string generate_unreal_entity();
OBJ_DATA * make_illusionary_object(const OBJ_DATA & source, int timer);
void revert_floating_disc_from_furniture(OBJ_DATA & disc);
bool try_convert_floating_disc_to_furniture(CHAR_DATA & ch, OBJ_DATA & disc);
void disc_update();

#endif
