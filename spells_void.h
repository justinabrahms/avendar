#ifndef SPELLS_VOID_H
#define SPELLS_VOID_H

#include "merc.h"

enum GrimseepAffiliation {Grimseep_None, Grimseep_Aided, Grimseep_Hindered};

GrimseepAffiliation check_grimseep_affiliation(const CHAR_DATA & ch);
GrimseepAffiliation check_grimseep_affiliation(const CHAR_DATA & ch, ROOM_INDEX_DATA & room);
void check_grimseep_update(ROOM_INDEX_DATA & room, AFFECT_DATA & paf);
int check_bloodofthevizier_count(CHAR_DATA & ch, CHAR_DATA & victim);
bool check_bloodofthevizier_drink(CHAR_DATA & ch, OBJ_DATA & obj);
void intensify_seedofmadness(CHAR_DATA & ch, AFFECT_DATA & saf);
void check_bierofunmaking(CHAR_DATA & victim);
void destroy_stasisrift(ROOM_INDEX_DATA & room, int targetVnum);
bool check_stasisrift(CHAR_DATA & ch, ROOM_INDEX_DATA & room);
bool is_in_stasis(CHAR_DATA & ch);
bool is_familiar_present(CHAR_DATA & ch, int sn, int obj_vnum);
bool check_webofoame_caught(CHAR_DATA & ch);
void check_webofoame_catch(CHAR_DATA & ch, ROOM_INDEX_DATA & room);
void check_eyeblighttouch(OBJ_DATA & obj, AFFECT_DATA & paf);
bool check_harvestofsouls(CHAR_DATA & ch);
bool check_wreathoffear(CHAR_DATA & ch, CHAR_DATA & victim);
void check_deathlyvisage(CHAR_DATA & ch, CHAR_DATA & victim);
bool is_nightstalking(const CHAR_DATA & ch);
void handleUpdateBarrowmist(ROOM_INDEX_DATA & room, AFFECT_DATA & paf);
void check_corpsesense_sight(CHAR_DATA & ch);
void check_direfeast(CHAR_DATA * ch, CHAR_DATA & victim, int dam);
void check_bonereaper(CHAR_DATA & victim);
void check_scionofnight(CHAR_DATA & ch);
void check_bonereaper_mana(CHAR_DATA & ch, int & mana);
bool check_idcizon_binding(CHAR_DATA * ch, bool & proceed);
void handle_phylactery_destruction(CHAR_DATA & ch, OBJ_DATA & obj);
bool should_trigger_corpsesense_hearing(ROOM_INDEX_DATA & room);
bool should_trigger_corpsesense_sight(ROOM_INDEX_DATA & room);
bool is_symbol_present(const ROOM_INDEX_DATA & room, int symbol_vnum);
int count_blackamulet(CHAR_DATA & ch);
CHAR_DATA * call_familiar(CHAR_DATA & ch, int sn, int level, int vnum);

#endif
