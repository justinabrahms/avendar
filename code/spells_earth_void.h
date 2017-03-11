#ifndef SPELLSEARTHVOID_H
#define SPELLSEARTHVOID_H

#include "merc.h"

int adjust_for_clockworksoul(CHAR_DATA * ch, CHAR_DATA & victim, int totalDamage);
bool deal_clockworksoul_damage(CHAR_DATA & ch, AFFECT_DATA & paf, int percent);
bool is_clockworkgolem_present(const ROOM_INDEX_DATA & room);
bool check_baneblade_modify_parry(OBJ_DATA & obj);
void check_baneblade_strike(CHAR_DATA & ch, CHAR_DATA & victim, OBJ_DATA & obj);

#endif
