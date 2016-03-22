#ifndef SPELLS_SPIRIT_AIR_H
#define SPELLS_SPIRIT_AIR_H

#include "merc.h"

void relocate_consciousness(CHAR_DATA * ch, CHAR_DATA * body);
CHAR_DATA * find_bilocated_body(CHAR_DATA * ch);
CHAR_DATA * find_bilocated_body(CHAR_DATA * ch, AFFECT_DATA * paf);
void check_mirrorofsouls(CHAR_DATA & ch, CHAR_DATA & victim);

#endif
