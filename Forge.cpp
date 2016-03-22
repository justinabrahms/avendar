#include "Forge.h"
#include "Luck.h"
#include "EchoAffect.h"
#include "NameMaps.h"
#include "recycle.h"
#include <set>
#include <sstream>
#include <memory>

static void AddBasicEffect(OBJ_DATA & obj, int location, int modifier)
{
    AFFECT_DATA af = {0};
    af.where    = TO_OBJECT;
    af.type     = -1;
    af.level    = obj.level;
    af.location = location;
    af.modifier = modifier;
    af.duration = -1;
    obj_affect_join(&obj, &af);
}

static bool AddTraitEffect(OBJ_DATA & obj, Forge::Trait trait)
{
    // Seek the existing trait
    for (AFFECT_DATA * paf(get_obj_affect(&obj, gsn_forgeweapon)); paf != NULL; paf = get_obj_affect(&obj, gsn_forgeweapon, paf))
    {
        if (paf->modifier == trait)
        {
            ++paf->level;
            return false;
        }
    }

    // Trait does not yet exist, so add it
    AFFECT_DATA af = {0};
    af.where    = TO_OBJECT;
    af.type     = gsn_forgeweapon;
    af.level    = 1;
    af.location = APPLY_HIDE;
    af.modifier = trait;
    af.duration = -1;
    affect_to_obj(&obj, &af);
    return true;
}

static void AddSphereEffect(OBJ_DATA & obj, Forge::Trait trait, const char * adjective)
{
    if (AddTraitEffect(obj, trait))
    {
        std::ostringstream mess;
        mess << adjective << ' ' << obj.obj_str;
        copy_string(obj.obj_str, mess.str().c_str());
    }

    if (number_bits(2) == 0) SET_BIT(obj.extra_flags[0], ITEM_MAGIC);
}

static const char * ChooseAdjective(const char ** adjectives, size_t count)
{
    return adjectives[number_range(0, count - 1)];
}

static void AddParry(OBJ_DATA & obj) {AddTraitEffect(obj, Forge::Trait_Parry);}
static void AddCasting(OBJ_DATA & obj) {AddTraitEffect(obj, Forge::Trait_Casting);}
static void AddNoDisarm(OBJ_DATA & obj) {SET_BIT(obj.extra_flags[0], ITEM_NODISARM);}
static void AddVampiric(OBJ_DATA & obj) {SET_BIT(obj.value[4], WEAPON_VAMPIRIC);}
static void AddPoison(OBJ_DATA & obj) {SET_BIT(obj.value[4], WEAPON_POISON);}
static void AddHitroll(OBJ_DATA & obj) {AddBasicEffect(obj, APPLY_HITROLL, 1);}
static void AddDamroll(OBJ_DATA & obj) {AddBasicEffect(obj, APPLY_DAMROLL, 1);}
static void AddSaves(OBJ_DATA & obj) {AddBasicEffect(obj, APPLY_SAVES, -1);}
static void AddLuck(OBJ_DATA & obj) {AddBasicEffect(obj, APPLY_LUCK, 1);}
static void AddHitPoints(OBJ_DATA & obj) {AddBasicEffect(obj, APPLY_HIT, 1);}
static void AddMana(OBJ_DATA & obj) {AddBasicEffect(obj, APPLY_MANA, 1);}
static void AddMoves(OBJ_DATA & obj) {AddBasicEffect(obj, APPLY_MOVE, 1);}

#define ADJECTIVE ChooseAdjective(adjectives, sizeof(adjectives)/sizeof(adjectives[0]))

static void AddWater(OBJ_DATA & obj) 
{
    static const char * adjectives[] = {"freezing", "frosty", "chill"};
    AddSphereEffect(obj, Forge::Trait_Water, ADJECTIVE);
    obj.value[3] = DAM_COLD;
}

static void AddEarth(OBJ_DATA & obj)
{
    AddTraitEffect(obj, Forge::Trait_Earth);
    if (number_bits(1) == 0) SET_BIT(obj.extra_flags[0], ITEM_NODESTROY);
    obj.weight += 5;
}

static void AddVoid(OBJ_DATA & obj)
{
    static const char * adjectives[] = {"unholy", "grim"};
    AddSphereEffect(obj, Forge::Trait_Void, ADJECTIVE);
    if (number_bits(2) == 0) SET_BIT(obj.extra_flags[0], ITEM_EVIL);
    if (number_bits(2) == 0) SET_BIT(obj.extra_flags[0], ITEM_DARK);

    if (obj.value[3] == DAM_NEGATIVE) obj.value[3] = DAM_DEFILEMENT;
    else obj.value[3] = DAM_NEGATIVE;
}

static void AddSpirit(OBJ_DATA & obj)
{
    static const char * adjectives[] = {"holy", "righteous", "smiting"};
    AddSphereEffect(obj, Forge::Trait_Spirit, ADJECTIVE);
    if (number_bits(2) == 0) SET_BIT(obj.extra_flags[0], ITEM_BLESS);
    if (number_bits(2) == 0) SET_BIT(obj.extra_flags[0], ITEM_GLOW);
    obj.value[3] = DAM_HOLY;
}

static void AddAir(OBJ_DATA & obj)
{
    static const char * adjectives[] = {"crackling", "sparking", "shocking"};
    AddSphereEffect(obj, Forge::Trait_Air, ADJECTIVE);
    if (number_bits(2) == 0) SET_BIT(obj.extra_flags[0], ITEM_HUM);
    obj.value[3] = DAM_LIGHTNING;
}

static void AddFire(OBJ_DATA & obj)
{
    static const char * adjectives[] = {"flaming", "burning", "scorching"};
    AddSphereEffect(obj, Forge::Trait_Fire, ADJECTIVE);
    if (number_bits(2) == 0) SET_BIT(obj.extra_flags[0], ITEM_GLOW);
    if (number_bits(2) == 0) SET_BIT(obj.extra_flags[0], ITEM_WARM);
    obj.value[3] = DAM_FIRE;
}

#undef ADJECTIVE

Forge::SpecialInfo::SpecialInfo(Special specialIn, int baseOddsIn, int costIn, bool multIn, bool uniqueClassIn, AddSpecialCallback callbackIn) :
    special(specialIn), baseOdds(baseOddsIn), cost(costIn), multipleAllowed(multIn), uniqueClass(uniqueClassIn), callback(callbackIn) {}

Forge::AffinityInfo::AffinityInfo(Special specialIn, Bias biasIn) :
    special(specialIn), bias(biasIn) {}

const Forge::SpecialMap & Forge::BuildSpecials()
{
    static SpecialMap specials;
    
    #define ADD_SPECIAL(specialIn, baseOddsIn, costIn, multipleIn, uniqueClassIn) \
        specials.insert(std::make_pair(specialIn, SpecialInfo(specialIn, baseOddsIn, costIn, multipleIn, uniqueClassIn, &Add##specialIn)))

    ADD_SPECIAL(Parry,      10, 30, false, false);
    ADD_SPECIAL(Casting,    10, 30, false, false);
    ADD_SPECIAL(NoDisarm,   20, 20, false, false);
    ADD_SPECIAL(Vampiric,    8, 30, false, false);
    ADD_SPECIAL(Poison,     20, 20, false, false);
    ADD_SPECIAL(Hitroll,    50,  5, true,  false);
    ADD_SPECIAL(Damroll,    30,  8, true,  false);
    ADD_SPECIAL(Saves,      40,  8, true,  false);
    ADD_SPECIAL(Luck,       10,  2, true,  false);
    ADD_SPECIAL(HitPoints,  50,  3, true,  false);
    ADD_SPECIAL(Mana,       30,  2, true,  false);
    ADD_SPECIAL(Moves,      20,  2, true,  false);
    ADD_SPECIAL(Water,       5, 40, true,  true);
    ADD_SPECIAL(Earth,       6, 40, true,  true);
    ADD_SPECIAL(Void,        4, 40, true,  true);
    ADD_SPECIAL(Spirit,      4, 40, true,  true);
    ADD_SPECIAL(Air,         5, 40, true,  true);
    ADD_SPECIAL(Fire,        6, 40, true,  true);

    #undef ADD_SPECIAL
    
    return specials;
}

const Forge::SpecialMap & Forge::Specials()
{
    static const SpecialMap & specials(BuildSpecials());
    return specials;
}

const Forge::WordList & Forge::BuildAdjectives()
{
    static WordList adjectives;
    #define ADD_ADJECTIVE(text) adjectives.push_back(text);

    ADD_ADJECTIVE("hardened");
    ADD_ADJECTIVE("tough");
    ADD_ADJECTIVE("well-balanced");
    ADD_ADJECTIVE("polished");
    ADD_ADJECTIVE("burnished");
    ADD_ADJECTIVE("hammered");
    ADD_ADJECTIVE("smooth");
    ADD_ADJECTIVE("beaten");

    #undef ADD_ADJECTIVE
    return adjectives;
}

const Forge::WordList & Forge::Adjectives()
{
    static const WordList & adjectives(BuildAdjectives());
    return adjectives;
}

const Forge::WeaponMap & Forge::BuildWeapons()
{
    static WeaponMap weapons;
    WeaponMap::iterator iter;

    #define ADD_WEAPON(weaponIn, typeIn, twohandedIn, damTypeIn, dammodIn, minWeightIn, objSizeIn)  \
        iter = weapons.insert(std::make_pair(weaponIn, WeaponInfo())).first;                        \
        iter->second.weapon = weaponIn; iter->second.name = #weaponIn;                              \
        iter->second.twohanded = twohandedIn; iter->second.damtype = damTypeIn;                     \
        iter->second.dammod = dammodIn; iter->second.type = typeIn;                                 \
        iter->second.minWeight = minWeightIn; iter->second.objSize = objSizeIn;                     \
        iter->second.name[0] = LOWER(iter->second.name[0])

    #define ADD_AFFINITY(specialIn, biasIn) \
        iter->second.affinities.insert(std::make_pair(specialIn, AffinityInfo(specialIn, biasIn)))

    #define ADD_DAMVERB(text) iter->second.damverbs.push_back(text)
    #define ADD_ADJECTIVE(text) iter->second.adjectives.push_back(text)
    #define ADD_SENTENCE(text) iter->second.sentences.push_back(text)

    ADD_WEAPON(Rapier, WEAPON_SWORD, false, DAM_PIERCE, 0, 50, SIZE_MEDIUM);
        ADD_DAMVERB("pierce");
        ADD_DAMVERB("skewering");
        ADD_DAMVERB("poke");
        ADD_DAMVERB("stab");
        ADD_ADJECTIVE("slender");
        ADD_ADJECTIVE("trim");
        ADD_ADJECTIVE("light");
        ADD_ADJECTIVE("elegant");
        ADD_ADJECTIVE("graceful");
        ADD_SENTENCE("Light yet strong, this quick blade has been designed to maximize agility in combat.");
        ADD_SENTENCE("The flexible blade ends in a polished cup, clearly built to shield the wielder's hand.");
        ADD_SENTENCE("The hilt has been bound with strips of leather, wound tightly to improve the grip.");
        ADD_AFFINITY(Parry, Likely);
        ADD_AFFINITY(Luck, Likely);
        ADD_AFFINITY(Hitroll, Guaranteed);
        ADD_AFFINITY(Damroll, Unlikely);
        ADD_AFFINITY(Casting, Prohibited);

    ADD_WEAPON(Shortsword, WEAPON_SWORD, false, DAM_SLASH, 0, 80, SIZE_MEDIUM);
        ADD_DAMVERB("slash");
        ADD_DAMVERB("slice");
        ADD_ADJECTIVE("thin");
        ADD_ADJECTIVE("thick");
        ADD_AFFINITY(NoDisarm, Likely);
        ADD_AFFINITY(Saves, Unlikely);
        ADD_AFFINITY(Casting, Prohibited);

    ADD_WEAPON(Broadsword, WEAPON_SWORD, false, DAM_SLASH, 1, 150, SIZE_MEDIUM);
        ADD_DAMVERB("slash");
        ADD_DAMVERB("slice");
        ADD_ADJECTIVE("wide");
        ADD_SENTENCE("The crossguard sports two narrow wings, designed to trap attacking blades.");
        ADD_SENTENCE("The angle of the blade is steep, crafted to maximize sharpness.");
        ADD_SENTENCE("A very slight curve has been worked into the blade, making it almost resemble a scimitar.");
        ADD_AFFINITY(Saves, Unlikely);
        ADD_AFFINITY(Casting, Prohibited);

    ADD_WEAPON(Claymore, WEAPON_SWORD, true, DAM_SLASH, 2, 210, SIZE_LARGE);
        ADD_DAMVERB("slash");
        ADD_DAMVERB("slice");
        ADD_DAMVERB("cleave");
        ADD_ADJECTIVE("massive");
        ADD_ADJECTIVE("mighty");
        ADD_SENTENCE("Attention was paid to balance during the forging of this blade, and it shows; despite its great size, the massive weapon remain quite wieldy.");
        ADD_SENTENCE("A slight sawtooth has been cut along the huge blade, designed to catch and tear flesh.");
        ADD_SENTENCE("The edge of the blade has been honed finely, matching its great heft with uncanny sharpness.");
        ADD_AFFINITY(Hitroll, Unlikely);
        ADD_AFFINITY(Damroll, Likely);
        ADD_AFFINITY(Saves, Unlikely);
        ADD_AFFINITY(Casting, Prohibited);
        ADD_AFFINITY(Moves, Prohibited);
        ADD_AFFINITY(Parry, Prohibited);
        
    ADD_WEAPON(Quarterstaff, WEAPON_STAFF, true, DAM_BASH, -1, 80, SIZE_MEDIUM);
        ADD_DAMVERB("smash");
        ADD_DAMVERB("crush");
        ADD_DAMVERB("whack");
        ADD_ADJECTIVE("thin");
        ADD_ADJECTIVE("stubby");
        ADD_SENTENCE("Two contoured grips have been cut into the staff, providing natural grooves for the wielder's hands.");
        ADD_SENTENCE("Each end of the rod bears a small metal knob, clearly intended to enhance striking power.");
        ADD_SENTENCE("A curling trail of ornamental metal winds about the staff.");
        ADD_AFFINITY(NoDisarm, Likely);
        ADD_AFFINITY(Saves, Likely);
        ADD_AFFINITY(Damroll, Unlikely);
        ADD_AFFINITY(Vampiric, Unlikely);
        ADD_AFFINITY(Poison, Prohibited);

    ADD_WEAPON(Staff, WEAPON_STAFF, true, DAM_BASH, 0, 140, SIZE_LARGE);
        ADD_DAMVERB("smash");
        ADD_DAMVERB("crush");
        ADD_DAMVERB("whack");
        ADD_ADJECTIVE("lengthy");
        ADD_ADJECTIVE("sturdy");
        ADD_AFFINITY(Saves, Likely);
        ADD_AFFINITY(Mana, Likely);
        ADD_AFFINITY(Vampiric, Unlikely);
        ADD_AFFINITY(Poison, Prohibited);

    ADD_WEAPON(Mace, WEAPON_MACE, false, DAM_BASH, 1, 110, SIZE_MEDIUM);
        ADD_DAMVERB("crush");
        ADD_DAMVERB("pound");
        ADD_DAMVERB("smash");
        ADD_ADJECTIVE("off-balance");
        ADD_AFFINITY(Damroll, Likely);
        ADD_AFFINITY(Hitroll, Unlikely);
        ADD_AFFINITY(Saves, Unlikely);
        ADD_AFFINITY(Casting, Prohibited);

    ADD_WEAPON(Morningstar, WEAPON_MACE, false, DAM_PIERCE, 1, 110, SIZE_MEDIUM);
        ADD_DAMVERB("rending");
        ADD_DAMVERB("mauling");
        ADD_ADJECTIVE("spiked");
        ADD_SENTENCE("The points have been finely-honed to pierce flesh or even mail.");
        ADD_SENTENCE("With smaller spikes and heftier head than the average morningstar, this one was designed to emphasize striking power.");
        ADD_SENTENCE("A leather grip helps prevent the weapon from being dragged out of the wielder's hand, should the spikes catch in armor.");
        ADD_AFFINITY(Damroll, Likely);
        ADD_AFFINITY(Hitroll, Unlikely);
        ADD_AFFINITY(Saves, Unlikely);
        ADD_AFFINITY(Casting, Prohibited);

    ADD_WEAPON(Warhammer, WEAPON_MACE, true, DAM_BASH, 2, 230, SIZE_LARGE);
        ADD_DAMVERB("crush");
        ADD_DAMVERB("mash");
        ADD_DAMVERB("smash");
        ADD_ADJECTIVE("massive");
        ADD_ADJECTIVE("mighty");
        ADD_SENTENCE("Several counterweights have been cleverly fused into the hammer at key points, helping to balance its powerful swing.");
        ADD_SENTENCE("With its extended grip, this hammer allows for unusually precise strikes for such a large weapon.");
        ADD_SENTENCE("The edges of the mace have been filed sharp to allow oblique strikes to more readily pierce armor.");
        ADD_AFFINITY(Damroll, Likely);
        ADD_AFFINITY(Saves, Unlikely);
        ADD_AFFINITY(Casting, Prohibited);

    ADD_WEAPON(Javelin, WEAPON_SPEAR, false, DAM_PIERCE, 0, 80, SIZE_MEDIUM);
        ADD_DAMVERB("stab");
        ADD_DAMVERB("thrust");
        ADD_ADJECTIVE("light");
        ADD_ADJECTIVE("slender");
        ADD_AFFINITY(Saves, Unlikely);
        ADD_AFFINITY(Casting, Prohibited);

    ADD_WEAPON(Spear, WEAPON_SPEAR, true, DAM_PIERCE, 1, 150, SIZE_LARGE);
        ADD_DAMVERB("thrust");
        ADD_DAMVERB("pierce");
        ADD_ADJECTIVE("long");
        ADD_ADJECTIVE("sturdy");
        ADD_AFFINITY(Saves, Unlikely);
        ADD_AFFINITY(Casting, Prohibited);

    ADD_WEAPON(Dirk, WEAPON_DAGGER, false, DAM_PIERCE, -1, 30, SIZE_SMALL);
        ADD_DAMVERB("poke");
        ADD_DAMVERB("stab");
        ADD_ADJECTIVE("small");
        ADD_AFFINITY(Vampiric, Likely);
        ADD_AFFINITY(Casting, Unlikely);

    ADD_WEAPON(Dagger, WEAPON_DAGGER, false, DAM_SLASH, 1, 40, SIZE_SMALL);
        ADD_DAMVERB("slash");
        ADD_DAMVERB("slice");
        ADD_ADJECTIVE("small");
        ADD_AFFINITY(Hitroll, Likely);
        ADD_AFFINITY(Casting, Unlikely);

    ADD_WEAPON(Halberd, WEAPON_POLEARM, true, DAM_SLASH, 2, 190, SIZE_LARGE);
        ADD_DAMVERB("carve");
        ADD_DAMVERB("swing");
        ADD_DAMVERB("chop");
        ADD_ADJECTIVE("long");
        ADD_ADJECTIVE("sharp");
        ADD_AFFINITY(Saves, Unlikely);
        ADD_AFFINITY(Casting, Prohibited);

    ADD_WEAPON(Lucerne, WEAPON_POLEARM, true, DAM_BASH, 2, 260, SIZE_LARGE);
        ADD_DAMVERB("smash");
        ADD_DAMVERB("crush");
        ADD_DAMVERB("pound");
        ADD_ADJECTIVE("unwieldy");
        ADD_AFFINITY(Saves, Unlikely);
        ADD_AFFINITY(Casting, Prohibited);

    ADD_WEAPON(Hatchet, WEAPON_AXE, false, DAM_SLASH, 0, 80, SIZE_MEDIUM);
        ADD_DAMVERB("chop");
        ADD_DAMVERB("hack");
        ADD_DAMVERB("cleave");
        ADD_ADJECTIVE("sharpened");
        ADD_ADJECTIVE("notched");
        ADD_AFFINITY(Saves, Unlikely);
        ADD_AFFINITY(Casting, Prohibited);

    ADD_WEAPON(Battleaxe, WEAPON_AXE, true, DAM_SLASH, 2, 180, SIZE_LARGE);
        ADD_DAMVERB("chop");
        ADD_DAMVERB("hack");
        ADD_DAMVERB("cleave");
        ADD_DAMVERB("carve");
        ADD_ADJECTIVE("massive");
        ADD_ADJECTIVE("mighty");
        ADD_AFFINITY(Damroll, Likely);
        ADD_AFFINITY(Saves, Unlikely);
        ADD_AFFINITY(Casting, Prohibited);

    ADD_WEAPON(Fallenstar, WEAPON_FLAIL, false, DAM_PIERCE, 1, 75, SIZE_MEDIUM);
        ADD_DAMVERB("laceration");
        ADD_DAMVERB("flailing");
        ADD_DAMVERB("flaying");
        ADD_ADJECTIVE("spiked");
        ADD_ADJECTIVE("flexible");
        ADD_SENTENCE("The chain links of this weapon split near the end into several sections, each topped with a spiked ball.");
        ADD_SENTENCE("Several trails of wickedly-spiked metal lead out the top of the fallenstar.");
        ADD_SENTENCE("Each sharp point at the end of the weapon has been tipped with a barb, to maximize tearing during a strike.");
        ADD_AFFINITY(Damroll, Likely);
        ADD_AFFINITY(Hitroll, Unlikely);
        ADD_AFFINITY(Saves, Unlikely);
        ADD_AFFINITY(Casting, Prohibited);

    ADD_WEAPON(Flail, WEAPON_FLAIL, false, DAM_SLASH, 1, 50, SIZE_MEDIUM);
        ADD_DAMVERB("flailing");
        ADD_DAMVERB("flaying");
        ADD_DAMVERB("tearing");
        ADD_ADJECTIVE("stiff");
        ADD_ADJECTIVE("banded");
        ADD_AFFINITY(Hitroll, Unlikely);
        ADD_AFFINITY(Saves, Unlikely);
        ADD_AFFINITY(Casting, Prohibited);

    ADD_WEAPON(Whip, WEAPON_WHIP, false, DAM_SLASH, 0, 30, SIZE_MEDIUM);
        ADD_DAMVERB("flaying");
        ADD_DAMVERB("whipping");
        ADD_ADJECTIVE("stiff");
        ADD_ADJECTIVE("oiled");
        ADD_SENTENCE("The metal links forming the weapon have been wound carefully together, with those at the end filed sharp.");
        ADD_SENTENCE("There is no obvious grip on this whip, which is little more than a well-crafted chain.");
        ADD_AFFINITY(Poison, Likely);
        ADD_AFFINITY(Hitroll, Unlikely);
        ADD_AFFINITY(Saves, Unlikely);
        ADD_AFFINITY(Casting, Prohibited);

    #undef ADD_SENTENCE
    #undef ADD_ADJECTIVE
    #undef ADD_DAMVERB
    #undef ADD_AFFINITY 
    #undef ADD_WEAPON

    return weapons;
}

const Forge::MaterialMap & Forge::BuildMaterials()
{
    static MaterialMap materials;
    MaterialMap::iterator iter;

    #define ADD_MATERIAL(materialName)                                              \
    {                                                                               \
        int materialNum(material_lookup(materialName));                             \
        if (materialNum < 0 || materialNum >= MAX_MATERIALS)                        \
        {                                                                           \
            bug("Invalid material specified in Forge::BuildMaterials", 0);          \
            return materials;                                                       \
        }                                                                           \
        iter = materials.insert(std::make_pair(materialNum, AffinityList())).first; \
    }

    #define ADD_AFFINITY(specialIn, biasIn) iter->second.push_back(AffinityInfo(specialIn, biasIn))

    ADD_MATERIAL("brass");
        ADD_AFFINITY(Vampiric, Likely);

	ADD_MATERIAL("bronze");
        ADD_AFFINITY(Luck, Likely);

	ADD_MATERIAL("copper");
        ADD_AFFINITY(Casting, Likely);

	ADD_MATERIAL("electrum");
        ADD_AFFINITY(Air, Likely);

	ADD_MATERIAL("elirium");
        ADD_AFFINITY(Spirit, Likely);
        ADD_AFFINITY(Vampiric, Prohibited);
        ADD_AFFINITY(Void, Prohibited);

	ADD_MATERIAL("gold");
        ADD_AFFINITY(Water, Prohibited);

	ADD_MATERIAL("iron");
        ADD_AFFINITY(Air, Prohibited);

	ADD_MATERIAL("lead");
        ADD_AFFINITY(Poison, Likely);
        ADD_AFFINITY(Water, Prohibited);

	ADD_MATERIAL("mei'tzec");
        ADD_AFFINITY(Earth, Likely);

	ADD_MATERIAL("mercury");
        ADD_AFFINITY(Parry, Likely);
        ADD_AFFINITY(Moves, Likely);

	ADD_MATERIAL("metal");

	ADD_MATERIAL("platinum");
        ADD_AFFINITY(HitPoints, Likely);

	ADD_MATERIAL("silver");
        ADD_AFFINITY(Saves, Likely);

	ADD_MATERIAL("steel");
        ADD_AFFINITY(Damroll, Likely);

	ADD_MATERIAL("tin");
        ADD_AFFINITY(Mana, Likely);

    #undef ADD_AFFINITY
    #undef ADD_MATERIAL

    return materials;
}

const Forge::MaterialMap & Forge::Materials()
{
    static const MaterialMap & materials(BuildMaterials());
    return materials;
}

const Forge::WeaponMap & Forge::Weapons()
{
    static const WeaponMap & weapons(BuildWeapons());
    return weapons;
}

unsigned int Forge::TraitCount(const OBJ_DATA & obj, Trait trait)
{
    // Seek the existing trait
    for (AFFECT_DATA * paf(get_obj_affect(&obj, gsn_forgeweapon)); paf != NULL; paf = get_obj_affect(&obj, gsn_forgeweapon, paf))
    {
        if (paf->modifier == trait)
            return paf->level;
    }

    return 0;
}

bool Forge::ObtainBaseObject(ForgeContext & context, std::vector<OBJ_DATA *> & baseObjects, CHAR_DATA & ch, const char *& argument)
{
    // Get the argument
    char arg[MAX_INPUT_LENGTH];
    argument = one_argument(argument, arg);
    if (arg[0] == '\0')
    {
        send_to_char("You must specify an object from which to forge the weapon.\n", &ch);
        return false;
    }

    // Get the object
    OBJ_DATA * obj(get_obj_carry(&ch, arg, &ch));
    if (obj == NULL)
    {
        // No such object
        std::ostringstream mess;
        mess << "You are carrying no " << arg << " from which to forge the " << context.weaponInfo->name << ".\n";
        send_to_char(mess.str().c_str(), &ch);
        return false;
    }

    // Check for a metal object
    if (!material_table[obj->material].metal)
    {
        act("You only know how to work metal, and thus cannot forge with $p.", &ch, obj, NULL, TO_CHAR);
        return false;
    }

    // Check for nodestroy
    if (IS_SET(obj->extra_flags[0], ITEM_NODESTROY))
    {
        act("$p is too tough to be melted down.", &ch, obj, NULL, TO_CHAR);
        return false;
    }

    // Check for specified already
    for (size_t i(0); i < baseObjects.size(); ++i)
    {
        if (obj == baseObjects[i])
        {
            act("You cannot alloy $p with itself!", &ch, obj, NULL, TO_CHAR);
            return false;
        }
    }

    // Object checks out
    baseObjects.push_back(obj);
    context.materials.push_back(obj->material);
    context.weight += obj->weight;
    return obj;
}

void Forge::CreateWeapon(CHAR_DATA & ch, const char * argument)
{
    // Perform basic skill check
    int skill(get_skill(&ch, gsn_forgeweapon));
    if (skill <= 0)
    {
        send_to_char("Huh?\n", &ch);
        return;
    }

    // Check for cooldown
    for (AFFECT_DATA * paf(get_affect(&ch, gsn_forgeweapon)); paf != NULL; paf = get_affect(&ch, gsn_forgeweapon, paf))
    {
        if (paf->point == NULL && paf->location == APPLY_NONE)
        {
            send_to_char("You are not ready for the trying process of forging another weapon just yet.\n", &ch);
            return;
        }
    }

    // Check for naming
    AFFECT_DATA * nameAff(FindNameEffect(ch));
    if (nameAff != NULL)
    {
        OBJ_DATA * nameObj(FindNameableObject(ch, *nameAff));
        if (nameObj != NULL)
        {
            act("You must first confer a true Name upon $p.", &ch, nameObj, NULL, TO_CHAR);
            return;
        }
        
        // No longer has the obj, so just clean up the effect and move on
        affect_remove(&ch, nameAff);
    }

    // Get the weapon type name
    char arg[MAX_INPUT_LENGTH];
    argument = one_argument(argument, arg);
    if (arg[0] == '\0')
    {
        send_to_char("Which type of weapon did you wish to forge?\n", &ch);
        return;
    }

    // Begin building the context, starting with the weapon type
    std::auto_ptr<ForgeContext> context(new ForgeContext);
    context->skill = skill;
    context->weaponInfo = LookupWeapon(arg);
    if (context->weaponInfo == NULL)
    {
        send_to_char("You do not know how to make weapons of that type.\n", &ch);
        return;
    }

    // Check for mana
    int manaCost(skill_table[gsn_forgeweapon].min_mana);
    if (ch.mana < manaCost)
    {
        send_to_char("You are too weary to forge a weapon right now.\n", &ch);
        return;
    }

    // Get the first object
    std::vector<OBJ_DATA*> baseObjects;
    if (!ObtainBaseObject(*context, baseObjects, ch, argument))
        return;

    // Check for an alloy
    argument = one_argument(argument, arg);
    if (arg[0] != '\0' && !str_prefix(arg, "alloy"))
    {
        if (!ObtainBaseObject(*context, baseObjects, ch, argument))
            return;

        argument = one_argument(argument, arg);
    }

    // Check for name
    if (arg[0] != '\0' && !str_prefix(arg, "name"))
    {
        // Verify max mana
        if (ch.max_mana < NamingManaCost)
        {
            send_to_char("You lack the will to speak a true Name.\n", &ch);
            return;
        }

        // Verify lava forge
        if (ch.in_room == NULL || !room_is_affected(ch.in_room, gsn_lavaforge))
        {
            send_to_char("You may only confer a true Name from the heat of the earth itself, at a lava forge.\n", &ch);
            return;
        }

        context->named = true;
        argument = one_argument(argument, arg);
    }

    // Check for superfluous arguments
    if (arg[0] != '\0')
    {
        send_to_char("Syntax: forgeweapon <type> <base_object> [alloy <base_object>] [name]\n", &ch);
        return;
    }

    // Check for sufficient weight
    if (context->weight < context->weaponInfo->minWeight)
    {
        std::ostringstream mess;
        mess << "Forging a " << context->weaponInfo->name << " requires more raw material than that!\n";
        send_to_char(mess.str().c_str(), &ch);
        return;
    }

    // Build object list to help with echoes
    std::ostringstream objMess;
    objMess << baseObjects[0]->short_descr;
    for (size_t i(1); (i + 1) < baseObjects.size(); ++i)
        objMess << ", " << baseObjects[i]->short_descr;

    if (baseObjects.size() > 1)
        objMess << " and " << baseObjects[baseObjects.size() - 1]->short_descr;

    std::string objList(objMess.str());

    // Perform initial echoes
    std::ostringstream mess;
    mess << "With a blast of heat and flame, you render down " << objList << " into a liquid mass of metal.";
    act(mess.str().c_str(), &ch, NULL, NULL, TO_CHAR);

    mess.str("");
    mess << "With a blast of heat and flame, $n renders down " << objList << " into a liquid mass of metal.";
    act(mess.str().c_str(), &ch, NULL, NULL, TO_ROOM);

    // Destroy the objects
    for (size_t i(0); i < baseObjects.size(); ++i)
        extract_obj(baseObjects[i]);

    // Set up the echo affect callbacks
    struct CallbackHandler
    {
        static bool HandleMove(CHAR_DATA * ch, ROOM_INDEX_DATA *, EchoAffect *, void * tag)
        {
            HandleCancel(ch, tag);
            return true;
        }

        static bool HandlePositionChange(CHAR_DATA * ch, int newPos, EchoAffect *, void * tag)
        {
            HandleCancel(ch, tag);
            return true;
        }

        static bool HandleCast(CHAR_DATA * ch, int, int, void *, int, EchoAffect *, void * tag)
        {
            HandleCancel(ch, tag);
            return true;
        }

        static bool CheckFailure(CHAR_DATA * ch, EchoAffect *, void * tag)
        {
            // Check for skill
            ForgeContext * context(static_cast<ForgeContext*>(tag));
            if (number_percent() <= context->skill)
            {    
                check_improve(ch, NULL, gsn_forgeweapon, true, 6);
                return false;
            }

            // Failed; increase the flaw count
            ++context->flaws;
            if (number_percent() <= context->flaws * 25)
            {
                // Fatal error
                act("You lose focus on your work, destroying it completely!", ch, NULL, NULL, TO_CHAR);
                act("$n loses focus on $s work, destroying it completely!", ch, NULL, NULL, TO_ROOM);
                check_improve(ch, NULL, gsn_forgeweapon, false, 6);
                delete context;
                return true;
            }

            // Non-fatal error
            act("You make a small error while folding the metal, introducing a slight flaw into the weapon.", ch, NULL, NULL, TO_CHAR);
            check_improve(ch, NULL, gsn_forgeweapon, false, 6);
            return false;
        }

        static bool Finish(CHAR_DATA * ch, EchoAffect *, void * tag)
        {
            std::auto_ptr<ForgeContext> context(static_cast<ForgeContext*>(tag));
            CompleteWeapon(*ch, *context);
            return false;
        }

        private:
            static void HandleCancel(CHAR_DATA * ch, void * tag)
            {
                std::auto_ptr<ForgeContext> context(static_cast<ForgeContext*>(tag));
                send_to_char("You abandon your efforts at the forge, letting the partially-worked metal run away to uselessness.\n", ch);
                act("$n abandons $s efforts at the forge, allowing the partially-worked metal to run away uselessly.", ch, NULL, NULL, TO_ROOM);
            }
    };

    // Prepare the echoAffect
    EchoAffect * echoAff(new EchoAffect(1));
    echoAff->SetPositionCallback(&CallbackHandler::HandlePositionChange);
    echoAff->SetMoveCallback(&CallbackHandler::HandleMove);
    echoAff->SetCastCallback(&CallbackHandler::HandleCast);

    // Add in lines
    echoAff->AddLine(&CallbackHandler::CheckFailure,
                    "Working the mass carefully, you begin to shape the metal.",
                    "Working the mass carefully, $n begins to shape the metal.");

    mess.str("");
    mess << "As you fold the metal over itself time and again, it starts to take on the distinctive shape of a ";
    mess << context->weaponInfo->name << ".";

    std::ostringstream omess;
    omess << "As $n folds the metal over itself time and again, it starts to take on the distinctive shape of a ";
    omess << context->weaponInfo->name << ".";

    echoAff->AddLine(&CallbackHandler::CheckFailure, mess.str().c_str(), omess.str().c_str());
    echoAff->AddLine(&CallbackHandler::CheckFailure,
                    "Molding the weapon with magic and skill, you bring your work near completion.",
                    "Molding the weapon with magic and skill, $n brings $s work near completion.");

    echoAff->AddLine(&CallbackHandler::Finish, "");

    // Finish applying the effect
    echoAff->SetTag(context.release());
    EchoAffect::ApplyToChar(&ch, echoAff);

    // Apply a cooldown
    AFFECT_DATA af = {0};
    af.where    = TO_AFFECTS;
    af.type     = gsn_forgeweapon;
    af.location = APPLY_NONE;
    af.duration = 60;
    af.level    = ch.level;
    affect_to_char(&ch, &af);

    // Charge mana and lag
    expend_mana(&ch, manaCost);
    WAIT_STATE(&ch, skill_table[gsn_forgeweapon].beats);
}

void Forge::CompleteWeapon(CHAR_DATA & ch, ForgeContext & context)
{
    // Determine the number of bonus points to spend
    bool forgeMaster(false);
    int bonusPoints(ch.level / 2);
    if (number_percent() <= get_skill(&ch, gsn_forgemaster)) 
    {
        check_improve(&ch, NULL, gsn_forgemaster, true, 4);
        forgeMaster = true; 
        bonusPoints += 10;
    }
    else
        check_improve(&ch, NULL, gsn_forgemaster, false, 4);

    if (ch.in_room != NULL && room_is_affected(ch.in_room, gsn_lavaforge)) bonusPoints += 20;
    if (context.named) bonusPoints += 60;
    bonusPoints -= context.flaws * 20;

    switch (Luck::Check(ch))
    {
        case Luck::Lucky: bonusPoints += 5; break;
        case Luck::Unlucky: bonusPoints -= 5; break;
        default: break;
    }

    bonusPoints = UMAX(0, bonusPoints);

    // Build the special odds ranges from the weapon and base objects
    std::vector<std::pair<unsigned int, const SpecialInfo *> > specials;
    unsigned int nextIndex(0);
    for (SpecialMap::const_iterator iter(Specials().begin()); iter != Specials().end(); ++iter)
    {
        // Make sure there are sufficient points for at least 1 level of this special
        if (bonusPoints < iter->second.cost)
            continue;

        // Calculate odds
        unsigned int oddsRange(CalculateOddsRange(iter->second, context.weaponInfo->weapon, context.materials));
        if (oddsRange == 0)
            continue;
        
        // Add in the range
        specials.push_back(std::make_pair(nextIndex, &iter->second));
        nextIndex += oddsRange;
    }

    // Now choose the special types, up to three
    std::vector<const SpecialInfo *> chosenSpecials;
    if (nextIndex > 0)
    {
        for (size_t i(0); i < 3; ++i)
        {
            // Choose an index at random and find the associated special
            unsigned int index(number_range(0, nextIndex - 1));
            for (size_t j(0); j + 1 < specials.size(); ++j)
            {
                if (specials[j].first <= index && specials[j + 1].first > index)
                {
                    // Found the matching special, now verify that it is acceptable in the face of other chosen ones
                    const SpecialInfo * chosenSpecial(specials[j].second);
                    for (size_t k(0); k < chosenSpecials.size(); ++k)
                    {
                        // No duplicates and no more than one uniqueclass
                        if (chosenSpecials[k] == chosenSpecial || (chosenSpecials[k]->uniqueClass && chosenSpecial->uniqueClass))
                        {
                            chosenSpecial = NULL;
                            break;
                        }
                    }

                    if (chosenSpecial != NULL)
                        chosenSpecials.push_back(chosenSpecial);

                    break;
                }
            }
        }
    }

    // Create the base object
    OBJ_DATA * obj(create_object(get_obj_index(OBJ_VNUM_FORGEWEAPON), 0));
    if (obj == NULL)
    {
        bug("Forge weapon failed due to bad obj creation", 0);
        send_to_char("An error has occurred, please contact the gods.\n", &ch);
        return;
    }

    // Fill out the basic info; use the first material as the object's material
    obj->level = ch.level;
    obj->weight = number_range(context.weaponInfo->minWeight, (context.weaponInfo->minWeight * 23) / 20);
    obj->weight = UMIN(obj->weight, context.weight);
    obj->size = context.weaponInfo->objSize;
    obj->cost = ((bonusPoints * bonusPoints) / 2) + number_range(0, 300);
    if (!context.materials.empty()) obj->material = context.materials[0];
    if (!context.weaponInfo->damverbs.empty()) 
        copy_string(obj->obj_str, context.weaponInfo->damverbs[number_range(0, context.weaponInfo->damverbs.size() - 1)].c_str());

    obj->value[0] = context.weaponInfo->type;
    obj->value[1] = 10 + (ch.level / 10) + context.weaponInfo->dammod + (context.named ? 1 : 0);
    obj->value[2] = 2;
    obj->value[3] = context.weaponInfo->damtype;
    if (context.weaponInfo->twohanded) SET_BIT(obj->value[4], WEAPON_TWO_HANDS);

    // Now choose from the selected specials at random until out of points or possible specials
    while (bonusPoints > 0 && !chosenSpecials.empty())
    {
        // Choose a special and add it in
        size_t index(number_range(0, chosenSpecials.size() - 1));
        chosenSpecials[index]->callback(*obj);
        bonusPoints -= UMAX(1, chosenSpecials[index]->cost);

        // Remove the special if too costly for the remaining points or only one allowed
        if (bonusPoints < chosenSpecials[index]->cost || !chosenSpecials[index]->multipleAllowed)
        {
            chosenSpecials[index] = chosenSpecials[chosenSpecials.size() - 1];
            chosenSpecials.pop_back();
        }
    }

    // Compensate for bad luck with increased damage
    if (context.named && bonusPoints >= 10)
        ++obj->value[1];

    // Choose an adjective from the set of common adjectives plus the weapon-specific adjectives
    WordList adjectives(Adjectives());
    for (size_t i(0); i < context.weaponInfo->adjectives.size(); ++i)
        adjectives.push_back(context.weaponInfo->adjectives[i]);

    std::string adjective;
    if (!adjectives.empty()) 
        adjective = adjectives[number_range(0, adjectives.size() - 1)];

    // Break down the material set to only unique materials
    std::set<int> materials;
    for (size_t i(0); i < context.materials.size(); ++i)
        materials.insert(context.materials[i]);

    std::string materialAdjective;
    switch (materials.size())
    {
        case 0: break;
        case 1: materialAdjective = material_table[context.materials[0]].name; break;
        case 2: 
        {
            std::set<int>::iterator iter(materials.begin());
            materialAdjective = material_table[*iter].name;
            materialAdjective += '-';
            ++iter;
            materialAdjective += material_table[*iter].name;
            break;
        }
        default:
            materialAdjective = material_table[context.materials[0]].name;
            materialAdjective += "-alloyed";
            break;
    }

    // Build the short desc
    std::ostringstream shortDesc;
    if (!adjective.empty()) shortDesc << adjective << ' ';
    if (!materialAdjective.empty()) shortDesc << materialAdjective << ' ';
    shortDesc << context.weaponInfo->name;

    std::string weaponShort(shortDesc.str());
    if (!weaponShort.empty())
        weaponShort = std::string(indefiniteArticleFor(weaponShort[0])) + ' ' + weaponShort;

    SetBasicDescs(*obj, weaponShort.c_str(), false);

    // Set up the exdesc
    std::ostringstream exdesc;
    switch (number_range(0, 2))
    {
        case 0: exdesc << "Forged"; break;
        case 1: exdesc << "Formed"; break;
        default: exdesc << "Wrought"; break;
    }
    exdesc << " from ";

    switch (materials.size())
    {
        case 0: exdesc << " pure metal"; break;
        case 1: exdesc << " pure " << material_table[context.materials[0]].name; break;
        case 2:
        {
            std::set<int>::iterator iter(materials.begin());
            exdesc << "an alloy of " << material_table[*iter].name << " and ";
            ++iter;
            exdesc << material_table[*iter].name;
            break;
        }
        default: exdesc << " a metal alloy"; break;
    }
    exdesc << ", this ";
    if (!adjective.empty()) exdesc << adjective << ' ';
    exdesc << context.weaponInfo->name;

    if (IS_OBJ_STAT(obj, ITEM_GLOW)) exdesc << " gleams brightly.";
    else if (IS_OBJ_STAT(obj, ITEM_DARK)) exdesc << " seems to drink the light into its dark surface.";
    else if (IS_OBJ_STAT(obj, ITEM_HUM)) exdesc << " hums faintly.";
    else if (IS_OBJ_STAT(obj, ITEM_NODESTROY)) exdesc << " seems nearly indestructible.";
    else if (context.named) exdesc << " radiates an aura of power.";
    else if (forgeMaster) exdesc << " has been masterfully crafted.";
    else exdesc << " has been well-crafted.";

    exdesc << ' ';
    switch (number_range(0, 5))
    {
        case 0: exdesc << "The bands of metal which comprise it have been folded over and over to toughen the final product."; break;
        case 1: exdesc << "Bands of metal flash-forged together serve to harden the weapon against wear."; break;
        case 2: exdesc << "Carved into the weapon's length are small straight-lined runes, etched with obvious care."; break;
        case 3: exdesc << "Folds of the metal have been beaten into the weapon's surface, reinforcing the craftsmanship."; break;
        case 4: exdesc << "Ornate filigree runs the length of the weapon, decorating it in gleaming metal."; break;
        default: exdesc << "The lines of the weapon are simple and unadorned, lending it a certain grim elegance."; break;
    }

    if (!context.weaponInfo->sentences.empty() && number_bits(1) == 0)
        exdesc << ' ' << context.weaponInfo->sentences[number_range(0, context.weaponInfo->sentences.size() - 1)];

    if (TraitCount(*obj, Trait_Parry) > 0)
        exdesc << " Forged with unusual precision, the balance of this piece lends itself well to parrying blows.";

    if (TraitCount(*obj, Trait_Casting) > 0)
        exdesc << " A sense of focus permeates this piece, as though it has been well-prepared for the channeling of mystical energies.";

    switch (context.flaws)
    {
        case 0: 
            switch (number_range(0, 3))
            {
                case 0: exdesc << " The work is flawless, bearing no sign of weakness."; break;
                case 1: exdesc << " Clearly well-fashioned, this " << context.weaponInfo->name << " has been made to stand the test of time."; break;
                case 2: exdesc << " No trace of imperfection mars the surface of this beautiful " << context.weaponInfo->name << '.'; break;
                default: exdesc << " There is no sign of seam or solder, as though the weapon was cast as a single, perfect unit."; break;
            }
            break;

        case 1:
            switch (number_range(0, 2))
            {
                case 0: exdesc << " A few minor imperfections mar the otherwise smooth surface of the " << context.weaponInfo->name << '.'; break;
                case 1: exdesc << " Though not immediately-visible, a couple of minor flaws are evident upon closer inspection."; break;
                default: exdesc << " Some dents remain from the original crafting, slightly weakening the metalwork."; break;
            }
            break;

        default:
            switch (number_range(0, 2))
            {
                case 0: exdesc << " Significant flaws are evident along the " << context.weaponInfo->name << "'s length."; break;
                default: exdesc << " Several seams show clearly on the weapon's surface, evidence of clumsy forgework."; break;
            }
            break;
    }

    // Apply the desc
    exdesc << '\n';
    EXTRA_DESCR_DATA * extraDesc(new_extra_descr());
    copy_string(extraDesc->keyword, obj->name);
    copy_string(extraDesc->description, exdesc.str().c_str());
    extraDesc->description = format_string(extraDesc->description);
    extraDesc->next     = obj->extra_descr;
    obj->extra_descr    = extraDesc;

    // Give the object to the char
    obj_to_char(obj, &ch);

    // Echoes (and charge max mana for the naming, if present)
    act("You quench the metalwork, now shaped into $p.", &ch, obj, NULL, TO_CHAR);
    act("$n quenches the metalwork, now shaped into $p.", &ch, obj, NULL, TO_ROOM);
    if (context.named)
    {
        act("You can feel the air about it charged with power drained from your very being, as the newly-forged weapon lies waiting to receive its true Name.", &ch, obj, NULL, TO_CHAR);
        ch.max_mana = UMAX(0, ch.max_mana - NamingManaCost);
        if (!IS_NPC(&ch))
            ch.pcdata->perm_mana = UMAX(0, ch.pcdata->perm_mana - NamingManaCost);

        ch.mana = UMIN(ch.mana, ch.max_mana);

        // Add an effect to the char to indicate he can name the object
        AFFECT_DATA af = {0};
        af.where    = TO_AFFECTS;
        af.type     = gsn_forgeweapon;
        af.location = APPLY_HIDE;
        af.duration = -1;
        af.point    = obj;
        af.level    = ch.level;
        affect_to_char(&ch, &af);
    }
}

void Forge::SetBasicDescs(OBJ_DATA & obj, const char * shortDesc, bool addComma)
{
    std::string originalName(obj.name);

    // Set up the name and long based on the short; strip any quotes or commas from the name
    std::string name;
    for (size_t i(0); shortDesc[i] != '\0'; ++i)
    {
        switch (shortDesc[i])
        {
            case '"':
            case ',':
                break;

            default:
                name += shortDesc[i];
                break;
        }
    }
    name += " obj_forgeweapon";
    
    std::string longDesc(shortDesc);
    if (addComma)
        longDesc += ',';
        
    longDesc += " lies here.";
    longDesc[0] = UPPER(longDesc[0]);

    // Now actually set the values
    setName(obj, name.c_str());
    copy_string(obj.short_descr, shortDesc);
    copy_string(obj.description, longDesc.c_str());

    // Now seek any extra descs based on the original name
    for (EXTRA_DESCR_DATA * ed(obj.extra_descr); ed != NULL; ed = ed->next)
    {
        if (originalName == ed->keyword)
            copy_string(ed->keyword, obj.name);
    }
}

void Forge::NameWeapon(CHAR_DATA & ch, const char * argument)
{
    // Check for name effect
    AFFECT_DATA * paf(FindNameEffect(ch));
    if (paf == NULL)
    {
        send_to_char("You have no weapon waiting to receive a Name.\n", &ch);
        return;
    }

    // Verify the object
    OBJ_DATA * obj(FindNameableObject(ch, *paf));
    affect_remove(&ch, paf);
    if (obj == NULL)
    {
        send_to_char("You are no longer carrying the weapon you empowered to receive a Name.\n", &ch);
        return;
    }

    // Object exists and is carried by the char, so name it according to the argument
    act("You pass your hand over $p, willing the Name into it.", &ch, obj, NULL, TO_CHAR);
    act("$n passes $s hand over $p, murmuring softly.", &ch, obj, NULL, TO_ROOM);

    // Echo about the naming
    std::ostringstream mess;
    mess << "A deep rush of power sweeps over $p as it claims the Name \"" << argument << "\"!";
    act(mess.str().c_str(), &ch, &obj, NULL, TO_ALL);

    // Add the name to the short desc
    std::string shortDesc(obj->short_descr);
    shortDesc += ", \"";
    shortDesc += argument;
    shortDesc += '"';
    SetBasicDescs(*obj, shortDesc.c_str(), true);

    // Create lore
    static char *his_her       [] = { "its", "his", "her" };
    const char * ch_name(IS_NPC(&ch) ? ch.short_descr : ch.name);
    std::ostringstream lore;
    lore << "Forged by the " << race_table[ch.race].name << " smith " << ch_name;
    lore << ", this mighty " << flag_string(weapon_class, obj->value[0]) << " was given its Name by the same.";
    lore << "Imbuing it with " << his_her[ch.sex] << " own power, " << ch_name << " marked \"" << argument;
    lore << "\" as an extension of " << his_her[ch.sex] << " very being.";
    copy_string(obj->lore, lore.str().c_str());
    obj->lore = format_string(obj->lore);
}

AFFECT_DATA * Forge::FindNameEffect(const CHAR_DATA & ch)
{
    for (AFFECT_DATA * paf(get_affect(&ch, gsn_forgeweapon)); paf != NULL; paf = get_affect(&ch, gsn_forgeweapon, paf))
    {
        if (paf->location == APPLY_HIDE)
            return paf;
    }

    return NULL;
}

OBJ_DATA * Forge::FindNameableObject(const CHAR_DATA & ch, const AFFECT_DATA & nameAffect)
{
    // Verify the object
    OBJ_DATA * obj(static_cast<OBJ_DATA*>(nameAffect.point));
    for (const OBJ_DATA * testObj(ch.carrying); testObj != NULL; testObj = testObj->next_content)
    {
        if (testObj == obj)
            return obj;
    }

    return NULL;
}

unsigned int Forge::CalculateOddsRange(const SpecialInfo & specialInfo, Weapon weapon, const std::vector<int> & materials)
{
    // Determine the bias
    Bias bias(DetermineWeaponBias(specialInfo.special, weapon));
    for (size_t i(0); i < materials.size(); ++i)
        bias = ResolveBiases(bias, DetermineMaterialBias(specialInfo.special, materials[i]));

    // Factor in the basic range for the special
    int range(specialInfo.baseOdds);
    switch (bias)
    {
        case Prohibited:    return 0;
        case Guaranteed:    return GuaranteedRange;
        case Likely:        range = (range * 2) + 20; break;
        case Unlikely:      range = (range / 2) - 20; break;
        default:            break;
    }

    return static_cast<unsigned int>(UMAX(range, 0));
}

Forge::Bias Forge::DetermineWeaponBias(Special special, Weapon weapon)
{
    WeaponMap::const_iterator weaponIter(Weapons().find(weapon));
    if (weaponIter == Weapons().end())
    {
        bug("Invalid weapon found in Forge::DetermineBias [%d]", weapon);
        return Normal;
    }

    AffinityMap::const_iterator iter(weaponIter->second.affinities.find(special));
    if (iter != weaponIter->second.affinities.end())
        return iter->second.bias;

    return Normal;
}

Forge::Bias Forge::DetermineMaterialBias(Special special, int material)
{
    // Check the material map for bias
    MaterialMap::const_iterator materialIter(Materials().find(material));
    if (materialIter == Materials().end())
        return Normal;
    
    // Run down the affinity list
    for (size_t i(0); i < materialIter->second.size(); ++i)
    {
        if (materialIter->second[i].special == special)
            return materialIter->second[i].bias;
    }

    return Normal;
}

Forge::Bias Forge::ResolveBiases(Bias lhs, Bias rhs)
{
    // Prohibited, then Guaranteed
    if (lhs == Prohibited || rhs == Prohibited) return Prohibited;
    if (lhs == Guaranteed || rhs == Guaranteed) return Guaranteed;

    // Unlikely; two Unlikelies makes a Prohibited
    if (lhs == Unlikely || rhs == Unlikely) 
        return ((lhs == rhs) ? Prohibited : Unlikely);
    
    // Likely; two Likelies makes a Guaranteed
    if (lhs == Likely || rhs == Likely) 
        return ((lhs == rhs) ? Guaranteed : Likely);

    return Normal;
}

const Forge::WeaponInfo * Forge::LookupWeapon(const char * name)
{
    // Look for a name prefix match
    for (WeaponMap::const_iterator iter(Weapons().begin()); iter != Weapons().end(); ++iter)
    {
        if (!str_prefix(name, iter->second.name.c_str()))
            return &iter->second;
    }

    return NULL;
}
