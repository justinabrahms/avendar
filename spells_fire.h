#ifndef SPELLSFIRE_H
#define SPELLSFIRE_H

#include "PyreInfo.h"
#include "fight.h"
#include <vector>
#include <utility>

void stopFiredancing(CHAR_DATA * ch, AFFECT_DATA * paf, int skill);
bool savesConsumeSpecial(CHAR_DATA * ch, CHAR_DATA * victim);
void checkApplySmoke(int level, CHAR_DATA * victim);
void fillRoomWithSmoke(CHAR_DATA * ch, ROOM_INDEX_DATA * room, int level, int duration);
PyreInfo * getPyreInfoEffect(CHAR_DATA * ch, PyreInfo::Effect effect);
std::vector<std::pair<CHAR_DATA*, PyreInfo*> > getPyreInfoEffectsArea(CHAR_DATA * ch, PyreInfo::Effect effect);
OBJ_DATA * lookupPyre(ROOM_INDEX_DATA * room);
CHAR_DATA * findCharForPyre(OBJ_DATA * pyre);
OBJ_DATA * lookupGroundFireHere(CHAR_DATA * ch);
std::vector<OBJ_DATA *> lookupGroundFires(CHAR_DATA * ch, OBJ_DATA * ignore, bool allowCursed);
void sourcelessDamage(CHAR_DATA * victim, const char * message, int amount, int sn, int dam_type);
void sourcelessDamage(CHAR_DATA * victim, const char * message, int sn, std::vector<DamageInfo> & damage);
void checkConflagration(CHAR_DATA * ch);
void checkAutoCauterize(CHAR_DATA * ch, int sn);
void applyCoronalGlow(CHAR_DATA * ch, CHAR_DATA * victim, int level, int duration, int amount);
bool checkDoorFireRune(CHAR_DATA * ch, int door);

#endif
