#include "Drakes.h"
#include "NameMaps.h"
#include <sstream>

Drakes::ResistInfo::ResistInfo(int typeIn, int modBaseIn, int modStepIn) :
    type(typeIn), modBase(modBaseIn), modStep(modStepIn) {}

Drakes::SpecialInfo::SpecialInfo(Special typeIn, Age ageIn, int amountIn) :
    type(typeIn), age(ageIn), amount(amountIn) {}

Drakes::Info::Info(int maturationModIn, int hpModIn, int damageModIn, int damageTypeIn) :
    maturationMod(maturationModIn), hpMod(hpModIn), damageMod(damageModIn), damageType(damageTypeIn) {}

int Drakes::AddInfo(MapType & infos, const char * name, const Info & info)
{
    // Lookup the type
    int stoneType(material_lookup(name));
    if (stoneType < 0 || stoneType >= MAX_MATERIALS || !material_table[stoneType].stone)
    {
        bug("Invalid stone name specified in Drakes::AddInfo", 0);
        exit(1);
    }

    // Check for duplicates
    if (infos.find(stoneType) != infos.end())
    {
        bug("Duplicate stone type specified in Drakes::AddInfo", 0);
        exit(1);
    }

    // Add the info
    infos.insert(std::make_pair(stoneType, info));
    return stoneType;
}

const Drakes::MapType & Drakes::BuildInfos()
{
    static MapType infos;
    int stoneType;
    Info * info;

    #define ADD_INFO(name, matMod, hpMod, damMod, damType) \
    stoneType = AddInfo(infos, (name), Info((matMod), (hpMod), (damMod), (damType))); \
    info = &infos.find(stoneType)->second

    #define ADD_RESIST(type, modBase, modStep) \
    info->resistances.push_back(ResistInfo((type), (modBase), (modStep)))

    #define ADD_SPECIAL(type, age, amount) \
    info->specials.push_back(SpecialInfo((type), (age), (amount)))

    ADD_INFO("diamond", 40, 40, 0, DAM_PIERCE);
        ADD_RESIST(RESIST_ENERGY, -25, 0);
        ADD_RESIST(RESIST_WEAPON, 10, 5);
        ADD_SPECIAL(Loyal, Hatchling, 25);
        ADD_SPECIAL(Diamondskin, Youngling, 2);
        ADD_SPECIAL(Loyal, Fledgling, 5);
        ADD_SPECIAL(Diamondskin, Wingborne, 1);
        ADD_SPECIAL(Loyal, Greater, 5);
        ADD_SPECIAL(Diamondskin, Elder, 1);
        ADD_SPECIAL(Loyal, Ancient, 5);
        ADD_SPECIAL(Diamondskin, Ancient, 1);

    ADD_INFO("granite", 50, 20, 10, DAM_BASH);
        ADD_RESIST(RESIST_MENTAL, -25, 0);
        ADD_RESIST(RESIST_BASH, 5, 5);
        ADD_SPECIAL(Bash, Hatchling, 1);
        ADD_SPECIAL(Bash, Youngling, 1);
        ADD_SPECIAL(Bash, Fledgling, 1);
        ADD_SPECIAL(Bash, Wingborne, 3);
        ADD_SPECIAL(Ferocious, Greater, 10);
        ADD_SPECIAL(Ferocious, Elder, 20);
        ADD_SPECIAL(Ferocious, Ancient, 30);

    ADD_INFO("quartz", 30, 20, 0, DAM_BASH);
        ADD_RESIST(RESIST_ACID, -25, 0);
        ADD_RESIST(RESIST_ENERGY, 5, 5);
        ADD_RESIST(RESIST_HOLY, 5, 5);
        ADD_RESIST(RESIST_LIGHT, 5, 5);
        ADD_SPECIAL(Overwhelm, Hatchling, 1);
        ADD_SPECIAL(Overwhelm, Youngling, 1);
        ADD_SPECIAL(Overwhelm, Fledgling, 1);
        ADD_SPECIAL(Overwhelm, Wingborne, 1);
        ADD_SPECIAL(Loyal, Wingborne, 10);
        ADD_SPECIAL(Overwhelm, Greater, 1);
        ADD_SPECIAL(Overwhelm, Elder, 1);
        ADD_SPECIAL(Overwhelm, Ancient, 1);

    ADD_INFO("obsidian", 20, -10, 0, DAM_PIERCE);
        ADD_RESIST(RESIST_SOUND, -25, 0);
        ADD_RESIST(RESIST_LIGHT, 0, 10);
        ADD_SPECIAL(Frantic, Youngling, 1);
        ADD_SPECIAL(Frantic, Wingborne, 1);
        ADD_SPECIAL(Frantic, Elder, 1);
        ADD_SPECIAL(Frantic, Ancient, 2);

    
    ADD_INFO("onyx", 20, 0, 0, DAM_BASH);
        ADD_RESIST(RESIST_SLASH, -25, 0);
        ADD_RESIST(RESIST_NEGATIVE, 5, 5);
        ADD_SPECIAL(Stonemend, Hatchling, 1);
        ADD_SPECIAL(Stonemend, Fledgling, 1);
        ADD_SPECIAL(Ambush, Wingborne, 1);
        ADD_SPECIAL(Stonemend, Greater, 1);
        ADD_SPECIAL(Stonemend, Ancient, 1);

    ADD_INFO("marble", 10, 10, 20, DAM_BASH);
        ADD_RESIST(RESIST_MAGIC, -25, 0);
        ADD_RESIST(RESIST_SLASH, 5, 5);
        ADD_SPECIAL(Stonemend, Hatchling, 1);
        ADD_SPECIAL(Stonemend, Youngling, 1);
        ADD_SPECIAL(Stonemend, Fledgling, 2);
        ADD_SPECIAL(Stonemend, Wingborne, 2);
        ADD_SPECIAL(Stonemend, Greater, 3);
        ADD_SPECIAL(Stonemend, Elder, 3);
        ADD_SPECIAL(Stonemend, Ancient, 4);
 
    ADD_INFO("jade", 0, -20, -10, DAM_SLASH);
        ADD_RESIST(RESIST_WEAPON, -25, 0);
        ADD_RESIST(RESIST_MAGIC, 10, 1);
        ADD_SPECIAL(Charge, Youngling, 20);
        ADD_SPECIAL(Charge, Fledgling, 20);
        ADD_SPECIAL(Charge, Wingborne, 20);
        ADD_SPECIAL(Charge, Greater, 20);
        ADD_SPECIAL(Charge, Elder, 20);
        ADD_SPECIAL(Ferocious, Ancient, 25);
 
    ADD_INFO("sapphire", 0, 0, 0, DAM_BASH);
        ADD_RESIST(RESIST_FIRE, -25, 0);
        ADD_RESIST(RESIST_COLD, 5, 5);
        ADD_SPECIAL(Loyal, Hatchling, 50);
        ADD_SPECIAL(Loyal, Youngling, 10);
        ADD_SPECIAL(Loyal, Fledgling, 10);
        ADD_SPECIAL(Loyal, Wingborne, 10);
        ADD_SPECIAL(Overwhelm, Wingborne, 2);
        ADD_SPECIAL(Loyal, Greater, 10);
        ADD_SPECIAL(Loyal, Elder, 10);
        ADD_SPECIAL(Loyal, Ancient, 50);
        ADD_SPECIAL(Overwhelm, Ancient, 1);
 
    ADD_INFO("ruby", 0, 0, 10, DAM_BASH);
        ADD_RESIST(RESIST_COLD, -25, 0);
        ADD_RESIST(RESIST_FIRE, 5, 5);
        ADD_SPECIAL(Vicious, Hatchling, 1);
        ADD_SPECIAL(Vicious, Youngling, 1);
        ADD_SPECIAL(Vicious, Fledgling, 2);
        ADD_SPECIAL(Vicious, Wingborne, 2);
        ADD_SPECIAL(Vicious, Greater, 3);
        ADD_SPECIAL(Vicious, Elder, 3);
        ADD_SPECIAL(Vicious, Ancient, 4);
 
    ADD_INFO("amethyst", 0, 10, -10, DAM_PIERCE);
        ADD_RESIST(RESIST_HOLY, -25, 0);
        ADD_RESIST(RESIST_NEGATIVE, 10, 2);
        ADD_SPECIAL(Defender, Hatchling, 10);
        ADD_SPECIAL(Ferocious, Youngling, 10);
        ADD_SPECIAL(Defender, Fledgling, 10);
        ADD_SPECIAL(Ferocious, Wingborne, 10);
        ADD_SPECIAL(Defender, Greater, 10);
        ADD_SPECIAL(Ferocious, Elder, 10);
        ADD_SPECIAL(Defender, Ancient, 10);
        ADD_SPECIAL(Ferocious, Ancient, 10);
 
    ADD_INFO("emerald", 0, 0, 0, DAM_BASH);
        ADD_RESIST(RESIST_WEAPON, 5, 1);
        ADD_RESIST(RESIST_MAGIC, 5, 1);
        ADD_SPECIAL(Loyal, Hatchling, 25);
        ADD_SPECIAL(Sunderer, Youngling, 5);
        ADD_SPECIAL(Sunderer, Fledgling, 10);
        ADD_SPECIAL(Sunderer, Wingborne, 15);
        ADD_SPECIAL(Sunderer, Greater, 20);
        ADD_SPECIAL(Sunderer, Elder, 25);
        ADD_SPECIAL(Sunderer, Ancient, 25);
 
    ADD_INFO("topaz", -10, 10, 0, DAM_BASH);
        ADD_RESIST(RESIST_LIGHTNING, -25, 0);
        ADD_RESIST(RESIST_WEAPON, 10, 0);
        ADD_SPECIAL(Loyal, Hatchling, 10);
        ADD_SPECIAL(Ferocious, Youngling, 20);
        ADD_SPECIAL(Ferocious, Fledgling, 20);
        ADD_SPECIAL(Ferocious, Wingborne, 20);
        ADD_SPECIAL(Ferocious, Greater, 20);
        ADD_SPECIAL(Ferocious, Elder, 20);
        ADD_SPECIAL(Overwhelm, Ancient, 1);
 
    ADD_INFO("opal", -10, -10, 0, DAM_BASH);
        ADD_RESIST(RESIST_NEGATIVE, -25, 0);
        ADD_RESIST(RESIST_ILLUSION, 30, 0);
        ADD_SPECIAL(Bash, Hatchling, 1);
        ADD_SPECIAL(Vicious, Youngling, 1);
        ADD_SPECIAL(Loyal, Fledgling, 10);
        ADD_SPECIAL(Overwhelm, Wingborne, 1);
        ADD_SPECIAL(Ferocious, Greater, 25);
        ADD_SPECIAL(Stonemend, Elder, 2);
        ADD_SPECIAL(Biting, Ancient, 2);
 
    ADD_INFO("bloodstone", -20, -40, 25, DAM_BASH);
        ADD_RESIST(RESIST_POISON, -25, 0);
        ADD_RESIST(RESIST_FIRE, -25, 0);
        ADD_SPECIAL(Biting, Hatchling, 2);
        ADD_SPECIAL(Biting, Youngling, 1);
        ADD_SPECIAL(Biting, Fledgling, 1);
        ADD_SPECIAL(Biting, Wingborne, 2);
        ADD_SPECIAL(Biting, Greater, 1);
        ADD_SPECIAL(Biting, Elder, 1);
        ADD_SPECIAL(Biting, Ancient, 2);
 
    ADD_INFO("aquamarine", -20, 0, 0, DAM_BASH);
        ADD_RESIST(RESIST_LIGHT, -25, 0);
        ADD_RESIST(RESIST_DROWNING, 50, 0);
        ADD_SPECIAL(Attentive, Hatchling, 1);
        ADD_SPECIAL(Attentive, Youngling, 2);
        ADD_SPECIAL(Attentive, Fledgling, 1);
        ADD_SPECIAL(Attentive, Wingborne, 2);
        ADD_SPECIAL(Attentive, Greater, 1);
        ADD_SPECIAL(Attentive, Elder, 2);
        ADD_SPECIAL(Attentive, Ancient, 3);
 
    ADD_INFO("shale", -30, 0, -20, DAM_SLASH);
        ADD_RESIST(RESIST_PIERCE, -25, 0);
        ADD_RESIST(RESIST_SLASH, 25, 0);
        ADD_SPECIAL(Defender, Youngling, 10);
        ADD_SPECIAL(Defender, Fledgling, 10);
        ADD_SPECIAL(Defender, Wingborne, 20);
        ADD_SPECIAL(Defender, Greater, 20);
        ADD_SPECIAL(Defender, Elder, 20);
        ADD_SPECIAL(Defender, Ancient, 20);
 
    ADD_INFO("sandstone", -40, -30, -10, DAM_SLASH);
        ADD_RESIST(RESIST_BASH, 25, 0);
        ADD_RESIST(RESIST_PIERCE, 5, 5);
        ADD_SPECIAL(Sandman, Hatchling, 2);
        ADD_SPECIAL(Sandman, Youngling, 1);
        ADD_SPECIAL(Sandman, Fledgling, 1);
        ADD_SPECIAL(Sandman, Wingborne, 2);
        ADD_SPECIAL(Sandman, Greater, 1);
        ADD_SPECIAL(Sandman, Elder, 1);
        ADD_SPECIAL(Sandman, Ancient, 2);
 
    #undef ADD_SPECIAL
    #undef ADD_RESIST
    #undef ADD_INFO

    return infos;
}

const Drakes::MapType & Drakes::Infos()
{
    static const MapType & infos(BuildInfos());
    return infos;
}

const Drakes::Info * Drakes::Lookup(int stoneType)
{
    // Seek the stoneType
    MapType::const_iterator iter(Infos().find(stoneType));
    if (iter == Infos().end())
        return NULL;

    // Found the stoneType
    return &iter->second;
}

void Drakes::WakenStone(CHAR_DATA & ch, const char * argument, int skill)
{
    // Check for location
    if (ch.in_room == NULL || !ON_GROUND(&ch))
    {
        send_to_char("You cannot reach the earth from this place.\n", &ch);
        return;
    }

    // Get the location's stone type and use it to lookup the drake info
    int stoneType(ch.in_room->stone_type);
    if (stoneType < 0) stoneType = ch.in_room->area->stone_type;
    const Info * info(Lookup(stoneType));
    if (info == NULL)
    {
        send_to_char("You probe your will into the earth, but cannot reach its stony heart from here.\n", &ch);
        return;
    }

    // Check whether an argument was provided
    if (argument[0] == '\0')
    {
        std::ostringstream mess;
        mess << "You probe your will into the earth, which resonates with a strong sense of ";
        mess << material_table[stoneType].name << ".\n";
        send_to_char(mess.str().c_str(), &ch);
        return;
    }

    // Check for an earth elemental pet present
    CHAR_DATA * elemental(get_char_room(&ch, argument));
    if (elemental == NULL)
    {
        send_to_char("You see nobody here by that name.\n", &ch);
        return;
    }

    // Verify the elemental
    if (ch.pet != elemental || elemental->master != &ch || !IS_NPC(elemental) || elemental->pIndexData->vnum != MOB_VNUM_EARTH_ELEMENTAL)
    {
        send_to_char("You may only waken an earth elemental which belongs to you.\n", &ch);
        return;
    }

    // Check for mana
    if (ch.mana < skill_table[gsn_wakenedstone].min_mana)
    {
        send_to_char("You lack the energy to waken stone right now.\n", &ch);
        return;
    }

    // Starting echoes
    act("You bind your will around $N, seeking a path from $M to the stony heart of the earth.", &ch, NULL, elemental, TO_CHAR);
    act("A steady murmur of energy flows back up the link, pouring into $N!", &ch, NULL, elemental, TO_CHAR);
    act("The ground begins to throb with energy, concentrated between $n and $N!", &ch, NULL, elemental, TO_ROOM);
    act("Shards of rock break and fall away from $M as you call out, waking the stone with your power!", &ch, NULL, elemental, TO_CHAR);
    act("$n calls out suddenly, and shards of rock break and fall away from $M!", &ch, NULL, elemental, TO_ROOM);

    // Check for skill success
    if (number_percent() <= skill)
    {
        // Make the base mob and adjust for age and type
        CHAR_DATA * drake(create_mobile(get_mob_index(MOB_VNUM_DRAKE)));
        AdjustDrake(ch, *drake, stoneType, *info, UMAX(ch.level, elemental->level), Hatchling);

        drake->hit = drake->max_hit;
        drake->mana = drake->max_mana;
        drake->move = drake->max_move;

        // Replace the elemental with the drake
        check_improve(&ch, NULL, gsn_wakenedstone, true, 1);
        char_to_room(drake, ch.in_room);
        ch.pet = drake;
        drake->master = &ch;
        drake->leader = &ch;

        // Success echo
        act("The last bits slough away, leaving behind $n.", drake, NULL, NULL, TO_ROOM);
    }
    else
    {
        // Failure echo
        act("You lose control, and $N crumbles away completely!", &ch, NULL, elemental, TO_CHAR);
        act("$N crumbles away completely, reduced to mere dust in moments.", &ch, NULL, elemental, TO_ROOM);
    }

    // Destroy the elemental
    elemental->master = NULL;
    extract_char(elemental, true);

    // Charge mana and lag
    expend_mana(&ch, skill_table[gsn_wakenedstone].min_mana);
    WAIT_STATE(&ch, skill_table[gsn_wakenedstone].beats);
}

bool Drakes::CheckMature(CHAR_DATA & drake, const AFFECT_DATA & paf)
{
    // Verify that this is a drake
    if (!IS_NPC(&drake) || drake.pIndexData->vnum != MOB_VNUM_DRAKE)
    {
        bug("Attempted to mature non-drake; somehow got wakened stone effect", 0);
        return false;
    }

    // Verify the drake's master
    if (drake.master == NULL || drake.in_room == NULL)
        return false;

    // Verify that this drake is not already ancient
    if (paf.level >= Ancient)
    {
        bug("Attempted to mature maxed drake [%d]", paf.level);
        return false;
    }

    // Lookup the stone info
    const Info * info(Lookup(paf.modifier));
    if (info == NULL)
    {
        bug("Attempted to mature drake with invalid stonetype %d", paf.modifier);
        return false;
    }

    // Adjust the drake and send echoes
    act("$n gives a keening wail as a steady throb of power courses over $m!", &drake, NULL, NULL, TO_ROOM);
    
    std::ostringstream mess;
    mess << "$s stony hide cracks and flexes in response, flaking off into chips of " << material_table[paf.modifier].name << ".";
    act(mess.str().c_str(), &drake, NULL, NULL, TO_ROOM);

    act("A new layer of mineral flesh hardens over the drake's frame, leaving $m noticeably larger than before!", &drake, NULL, NULL, TO_ROOM);
    AdjustDrake(*drake.master, drake, paf.modifier, *info, UMAX(drake.master->level, drake.level - paf.level), static_cast<Age>(paf.level + 1));
    return true;
}

void Drakes::AdjustDrake(CHAR_DATA & ch, CHAR_DATA & drake, int stoneType, const Info & info, int level, Age age)
{
    // Set the appropriate strings
    const char * stoneName(material_table[stoneType].name);
    std::string shortDesc(BuildShortDesc(stoneName, age));
    std::string longDesc(shortDesc + " is here.\n");
    longDesc[0] = UPPER(longDesc[0]);
    setName(drake, shortDesc.c_str());
    copy_string(drake.short_descr, shortDesc.c_str());
    copy_string(drake.long_descr, longDesc.c_str());
    copy_string(drake.description, BuildDescription(ch, stoneName, age).c_str());

    // Seed any random values with ch's id
    srand(ch.id + 12345);

    // Adjust damage type and damverb
    drake.dam_type = info.damageType;
    const char * damverb("strike");
    switch (info.damageType)
    {
        case DAM_BASH:
            switch ((rand() + stoneType) % 3)
            {
                case 0: damverb = "slam"; break;
                case 1: damverb = "smash"; break;
                case 2: damverb = "crush"; break;
            }
            break;

        case DAM_SLASH:
            switch ((rand() + stoneType) % 3)
            {
                case 0: damverb = "slice"; break;
                case 1: damverb = "claw"; break;
                case 2: damverb = "talon"; break;
            }
            break;
 
        case DAM_PIERCE:
            switch ((rand() + stoneType) % 2)
            {
                case 0: damverb = "bite"; break;
                case 1: damverb = "strike"; break;
            }
            break;
    }
    copy_string(drake.dam_verb, damverb);

    // Perform basic adjustments
    drake.level = UMAX(1, level + age);
    drake.damroll = (level / 10) + (age * 4);
    drake.hitroll = (level * 2) / 3;
    drake.damage[0] = (level * 2) / 3;
    drake.damage[1] = 4;
    drake.damage[2] = drake.damroll;
    drake.max_hit = dice(level * 4, 19) + 400 + (age * 100);

    // Adjust for damage and hp mods
    drake.damage[0] = (drake.damage[0] * (100 + info.damageMod)) / 100;
    drake.max_hit = (drake.max_hit * (100 + info.hpMod)) / 100;

    // Adjust stats
    for (size_t i(0); i < MAX_STATS; ++i)
    {
        drake.perm_stat[i] = 18 + age;
        drake.mod_stat[i] = 18 + age;
        drake.max_stat[i] = 18 + age;
    }

    // Fill in resistances
    for (size_t i(0); i < info.resistances.size(); ++i)
        drake.resist[info.resistances[i].type] = info.resistances[i].modBase + (age * info.resistances[i].modStep);

    // Restore random seed
    srand(time(0));

    // Determine duration until next age level up
    int duration((age + 1) * 260);
    if (age == Ancient) duration = -1;
    else duration = (duration * (100 + info.maturationMod)) / 100;

    // Mark the drake's stone type and age in the modifier and level fields
    AFFECT_DATA af = {0};
    af.where    = TO_AFFECTS;
    af.type     = gsn_wakenedstone;
    af.modifier = stoneType;
    af.level    = age;
    af.duration = duration;
    affect_to_char(&drake, &af);

    // Adjust the drake's level for overwhelm
    drake.level += SpecialCount(drake, Overwhelm);
}

int Drakes::SpecialCount(CHAR_DATA & drake, Special special)
{
    // Verify that this is a drake
    if (!IS_NPC(&drake) || drake.pIndexData->vnum != MOB_VNUM_DRAKE)
        return 0;

    // Find the wakened stone effect
    AFFECT_DATA * paf(get_affect(&drake, gsn_wakenedstone));
    if (paf == NULL)
        return 0;

    // Get the stone type from the effect and look up the info
    const Info * info(Lookup(paf->modifier));
    if (info == NULL)
    {
        bug("Failed to find stone in Drakes::SpecialCount [%d]", paf->modifier);
        return 0;
    }

    // Run down the specials, tallying them up
    int total(0);
    for (size_t i(0); i < info->specials.size(); ++i)
    {
        if (info->specials[i].type == special && info->specials[i].age <= paf->level)
            total += info->specials[i].amount;
    }

    return total;
}

bool Drakes::IsMinimumAge(const CHAR_DATA & drake, Age age)
{
    // Verify that this is a drake
    if (!IS_NPC(&drake) || drake.pIndexData->vnum == MOB_VNUM_DRAKE)
        return false;

    // Find the wakened stone effect
    AFFECT_DATA * paf(get_affect(&drake, gsn_wakenedstone));
    if (paf == NULL)
        return false;

    // Check the age
    return (paf->level >= age);
}

bool Drakes::HasStonesong(const CHAR_DATA & drake)
{
    // Verify that this is a drake
    if (!IS_NPC(&drake) || drake.pIndexData->vnum != MOB_VNUM_DRAKE)
        return false;

    // Check for a drake master present
    if (drake.master == NULL || drake.master->in_room != drake.in_room)
        return false;

    // Check for stonesong
    return is_affected(drake.master, gsn_conduitofstonesong);
}

std::string Drakes::BuildShortDesc(const char * stoneName, Age age)
{
    std::ostringstream shortDescr;

    // Determine noun and adjective
    const char * noun(NULL);
    const char * adjective(NULL);
    switch (age)
    {
        case Hatchling: noun = "hatchling";         break;
        case Youngling: noun = "youngling";         break;
        case Fledgling: adjective = "fledgling";    break;
        case Wingborne:                             break;
        case Greater:   adjective = "greater";      break;
        case Elder:     adjective = "elder";        break;
        case Ancient:   adjective = "ancient";      break;
    }

    // Start with the article and adjective
    if (adjective == NULL) shortDescr << indefiniteArticleFor(stoneName[0]);
    else shortDescr << indefiniteArticleFor(adjective[0]) << ' ' << adjective;

    // Add in the stone type and noun
    shortDescr << ' ' << stoneName << " drake";
    if (noun != NULL)
        shortDescr << ' ' << noun;

    return shortDescr.str();
}

std::string Drakes::BuildDescription(CHAR_DATA & ch, const char * stoneName, Age age)
{
    std::ostringstream result;
    
    // Devise a random seed based on caster and stone type
    unsigned int seed(ch.id);
    for (size_t i(0); stoneName[i] != '\0'; ++i)
        seed += stoneName[i];

    // Determine the random words
    const char * strata("banded with");
    srand(seed);
    switch (number_range(0, 4))
    {
        case 0: strata = "banded with"; break;
        case 1: strata = "lined in"; break;
        case 2: strata = "shot through with streaks of"; break;
        case 3: strata = "speckled with"; break;
        case 4: strata = "marked with bands of"; break;
    }
    srand(time(0));

    switch (age)
    {
        case Hatchling:
            result << "A drake hatchling is here, " << strata << ' ' << stoneName << ". ";
            result << "Pale scrapings of the stone curl all over its mineral flesh, lending it an almost downy appearance. ";
            result << "With its tiny fangs and complete absence of wings, the drake seems to be quite young.";
            break;

        case Youngling:
            result << "A drake youngling is here, " << strata << ' ' << stoneName << ". ";
            result << "The early stubs of its wings have begun to develop, tipped in small claws. ";
            result << "The " << stoneName << " in its flesh is starting to show early signs of deepening in hue.";
            break;

        case Fledgling:
            result << "A drake fledgling is here, " << strata << ' ' << stoneName << ". ";
            result << "Though still clearly a juvenile, the plates of stone which make up its flesh have ";
            result << "taken on nearly the deeper hue associated with adults of its kind. ";
            result << "Claw-tipped wings sprout from its sides, almost fully-developed.";
            break;

        case Wingborne:
            result << "A full-grown drake is here, " << strata << ' ' << stoneName << ". ";
            result << "Sharp fangs protrude from its maw, complementing the curved claws extending from its wings. ";
            result << "Stony scale plates armor its body, their weight balanced by a ridged tail.";
            break;

        case Greater:
            result << "A greater drake is here, " << strata << ' ' << stoneName << ". ";
            result << "Though similar in appearance to the rest of its kind, this beast is unusually large, bearing ";
            result << "pronounced fangs and powerful claw-tipped wings.";
            break;

        case Elder:
            result << "An elder drake is here, " << strata << ' ' << stoneName << ". ";
            result << "Thick, tough plates of stone armor adorn its body, protruding out in spikes at sharp angles. ";
            result << "Its eyes are dark and focused, taking in the world about it with their steely gaze."; 
            break;

        case Ancient:
            result << "An ancient drake is here, " << strata << ' ' << stoneName << ". ";
            result << "The thick plates of its stony hide have fused together over time and sprouted sharp spikes, which ";
            result << "jut out at a odd angles from its body and tail. With powerful fangs and long claws on the ends of ";
            result << "mighty wings, this beast seems formidable indeed.";
            break;
    }

    // Format the string before returning
    // All the memory manipulation issues are to support the awkward API of format_string
    char * descIn(str_dup(result.str().c_str()));
    char * resultDesc(format_string(descIn));
    std::string desc(resultDesc);
    free_string(resultDesc);
    return desc;
}
