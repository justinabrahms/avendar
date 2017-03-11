#ifndef SPELLSSPIRIT_H
#define SPELLSSPIRIT_H

#include "merc.h"
#include <vector>

enum LitanyType {Litany_None = -1, Litany_Benediction = 0, Litany_Mortification = 1, Litany_Purification = 2};
enum SpiritAura {Aura_Soothing = 0, Aura_Empathy = 1, Aura_Genesis = 2};

bool is_an_avatar(CHAR_DATA * ch);
int type_of_avatar(CHAR_DATA * ch);
void strip_avatar(CHAR_DATA * ch);

const char * litanyName(LitanyType litany);
bool perform_litany_skill_check(CHAR_DATA * ch, CHAR_DATA * victim, LitanyType litany, int divisor);
int lethebane_sleep_level_mod(CHAR_DATA * ch, CHAR_DATA * victim);
void apply_runeofspirit_cooldown(CHAR_DATA * ch);

bool is_demon(CHAR_DATA * ch);
const char * auraBaseName(SpiritAura aura);
std::vector<CHAR_DATA*> MembersOfAetherealCommunion(CHAR_DATA * ch, unsigned int & bestModifier);
void handle_quintessence_destruction(CHAR_DATA * ch, OBJ_DATA * obj);
std::vector<OBJ_DATA*> find_posterns_for(long id);
void handle_remove_postern(OBJ_DATA * postern, const std::vector<OBJ_DATA*> & posterns);
bool is_quintessence_rushing(CHAR_DATA * ch);
AFFECT_DATA * get_quintessence_rushing_affect(CHAR_DATA * ch);
void summon_avenging_seraph(int level, CHAR_DATA * target);
void tryApplyPacification(int level, int duration, CHAR_DATA * ch, CHAR_DATA * victim, bool areaWide);
OBJ_DATA * lookup_obj_extra_flag(CHAR_DATA * ch, int extraFlag, OBJ_DATA * lastObj = NULL);

#endif
