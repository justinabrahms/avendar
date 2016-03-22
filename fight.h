#ifndef FIGHT_H
#define FIGHT_H

#include "merc.h"
#include <vector>

struct DamageInfo
{
    DamageInfo();
    DamageInfo(int amountIn, int typeIn);

    int amount;
    int type;
};

inline DamageInfo::DamageInfo() : amount(0), type(DAM_OTHER) {}
inline DamageInfo::DamageInfo(int amountIn, int typeIn) : amount(amountIn), type(typeIn) {}

bool damage_new(CHAR_DATA *ch, CHAR_DATA *victim, std::vector<DamageInfo> damage, int dt, bool show, char *attack = NULL, const char * attackerName = NULL);
void dam_message(CHAR_DATA *ch, CHAR_DATA *victim, int dam, int dt, char *amess, bool immune);

extern std::vector<DamageInfo> global_showdam;

#endif
