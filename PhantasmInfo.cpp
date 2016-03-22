#include "PhantasmInfo.h"
#include <cmath>
#include <sstream>
#include <memory>
#include <iomanip>
#include "interp.h"
#include "RomanNumerals.h"
#include "NameMaps.h"

void PhantasmInfo::serializeInfo(std::ostream & out, const Info & info) const
{
    out << info.vnum << ' ';
    out << info.promotions;

    for (size_t i(0); i < info.traits.size(); ++i)
    {
        out << ' ' << static_cast<unsigned int>(info.traits[i].trait);
        out << ' ' << info.traits[i].count;
    }
}

std::string PhantasmInfo::serialize() const
{
    std::ostringstream result;
    for (size_t i(0); i < m_phantasms.size(); ++i)
    {
        serializeInfo(result, m_phantasms[i]);
        result << '|';
    }
    return result.str();
}

bool PhantasmInfo::deserializeInfo(Info & info, const char * buffer, size_t length)
{
    // Parse the base values
    std::istringstream in(std::string(buffer, length));
    in >> info.vnum >> info.promotions;
    if (in.fail())
        return false;

    // Parse the traits
    while (!in.eof())
    {
        unsigned int traitVal;
        in >> traitVal;
        if (traitVal >= PhantasmTrait::Max)
            return false;

        info.traits.push_back(PhantasmTrait::TakenInfo(static_cast<PhantasmTrait::Trait>(traitVal)));
        in >> info.traits[info.traits.size() - 1].count;
        if (in.fail())
            return false;
    }

    return true;
}

PhantasmInfo * PhantasmInfo::deserialize(const char * buffer)
{
    std::auto_ptr<PhantasmInfo> result(new PhantasmInfo);

    while (buffer[0] != '\0')
    {
        // Find the divider used to mark the end of every Info
        const char * delimiter(strchr(buffer, '|'));
        if (delimiter == NULL)
            return NULL;

        // Deserialize the Info
        result->m_phantasms.push_back(Info());
        if (!deserializeInfo(result->m_phantasms[result->m_phantasms.size() - 1], buffer, delimiter - buffer))
            return NULL;

        // Advance the string
        buffer = delimiter + 1;
    }

    return result.release();
}

void PhantasmInfo::copyMobString(char *& destination, const char * source)
{
    free_string(destination);
    destination = str_dup(source);
}

unsigned int PhantasmInfo::lookupIndexByName(const char * name) const
{
    unsigned int len(strlen(name));
    unsigned int possibleMatch(NotFound);
    bool ambiguous(false);
    for (size_t i(0); i < m_phantasms.size(); ++i)
    {
        // Lookup the mob index
        const MOB_INDEX_DATA * mobIndex(lookupMobIndex(m_phantasms[i].vnum));
        if (mobIndex == NULL)
            continue;

        // Compare the names; on exact match, return immediately
        if (!str_cmp(name, mobIndex->player_name))
            return i;

        // Check for possible matches
        if (len >= 3 && (!str_prefix(name, mobIndex->player_name) || !str_infix(name, mobIndex->player_name)))
        {
            if (possibleMatch == NotFound)
                possibleMatch = i;
            else
                ambiguous = true;
        }
    }

    if (ambiguous)
        return NotFound;

    return possibleMatch;
}

const MOB_INDEX_DATA * PhantasmInfo::lookupMobIndex(int vnum)
{
    MOB_INDEX_DATA * mobIndex(get_mob_index(vnum));
    if (mobIndex == NULL)
    {
        std::ostringstream mess;
        mess << "PhantasmInfo: failed due to invalid mob index " << vnum;
        bug(mess.str().c_str(), 0);
        return NULL;
    }

    return mobIndex;
}

unsigned int PhantasmInfo::lookupIndexByVnum(int vnum) const
{
    for (size_t i(0); i < m_phantasms.size(); ++i)
    {
        if (m_phantasms[i].vnum == vnum)
            return i;
    }

    return NotFound;
}

const PhantasmInfo::Info * PhantasmInfo::lookupVnum(int vnum) const
{
    unsigned int index(lookupIndexByVnum(vnum));
    if (index == NotFound)
        return NULL;

    return &m_phantasms[index];
}

const PhantasmInfo::Info * PhantasmInfo::lookupInfo(const CHAR_DATA & ch)
{
    // Check for phantasm
    if (!IS_NPC(&ch) || ch.pIndexData->vnum != MOB_VNUM_PHANTASMALMIRROR)
        return NULL;

    // Check for master
    if (ch.master == NULL || IS_NPC(ch.master) || ch.master->pcdata->phantasmInfo == NULL)
        return NULL;

    // Find the phantasm in the list
    return (ch.master->pcdata->phantasmInfo->lookupVnum(ch.mobvalue[MobSlotInfoIndex]));
}

unsigned int PhantasmInfo::traitCount(const CHAR_DATA & ch, PhantasmTrait::Trait trait)
{
    // Find the info
    const Info * info(lookupInfo(ch));
    if (info == NULL)
        return 0;

    // Find the trait in the phantasm info
    for (size_t i(0); i < info->traits.size(); ++i)
    {
        if (info->traits[i].trait == trait)
            return info->traits[i].count;
    }

    // Did not find trait
    return 0;
}

const MOB_INDEX_DATA * PhantasmInfo::baseMobIndex(const CHAR_DATA & ch)
{
    // Find the info
    const Info * info(lookupInfo(ch));
    if (info == NULL)
        return NULL;

    // Find the mob index
    return lookupMobIndex(info->vnum);
}

bool PhantasmInfo::empowerPhantasm(CHAR_DATA & ch, int level, unsigned int index, const char * argument)
{
    // Lookup the mob index
    Info & info(m_phantasms[index]);
    const MOB_INDEX_DATA * mobIndex(lookupMobIndex(info.vnum));
    if (mobIndex == NULL)
    {
        send_to_char("An error has occurred; please contact the Immortals.\n", &ch);
        return false;
    }

    // Check for cooldown
    std::ostringstream mess;
    for (const AFFECT_DATA * paf(get_affect(&ch, gsn_empowerphantasm)); paf != NULL; paf = get_affect(&ch, gsn_empowerphantasm, paf))
    {
        if (paf->modifier == info.vnum)
        {
            mess << "You are not yet ready to empower " << mobIndex->short_descr << " again.\n";
            send_to_char(mess.str().c_str(), &ch);
            return false;
        }
    }

    // Lookup the trait
    PhantasmTrait::Trait trait(PhantasmTrait::TraitFor(argument));
    if (trait == PhantasmTrait::Max)
    {
        mess << "Unrecognized empowerment '" << argument << "'.\n";
        send_to_char(mess.str().c_str(), &ch);
        return false;
    }

    // Check the affinity
    PhantasmTrait::Affinity affinity(PhantasmTrait::AffinityFor(trait, *mobIndex, info.traits, true));
    if (affinity == PhantasmTrait::Absurd)
    {
        mess << "You cannot give the " << PhantasmTrait::NameFor(trait) << " empowerment to " << mobIndex->short_descr << ".\n";
        send_to_char(mess.str().c_str(), &ch);
        return false;
    }

    // Give the empowerment; increment any existing, or add a new one if not already present
    bool found(false);
    for (size_t i(0); i < info.traits.size(); ++i)
    {
        if (trait == info.traits[i].trait)
        {
            ++info.traits[i].count;
            found = true;
            break;
        }
    }

    if (!found)
        info.traits.push_back(PhantasmTrait::TakenInfo(trait));

    // Apply cooldown
    AFFECT_DATA af = {0};
    af.where    = TO_AFFECTS;
    af.type     = gsn_empowerphantasm;
    af.level    = level;
    af.duration = calculateNextCooldown(makeSeed(ch.id, info.vnum), trait, info.promotions, affinity);
    af.modifier = info.vnum;
    affect_to_char(&ch, &af);

    // Update promotions count to increase the next cooldown for this phantasm
    ++info.promotions;

    // Send echoes
    mess << "You focus your will into the phantasmal strength of " << mobIndex->short_descr << ", empowering it with illusory magics!\n";
    send_to_char(mess.str().c_str(), &ch);
    return true;
}

unsigned int PhantasmInfo::makeSeed(int casterID, int vnum)
{
    return (static_cast<unsigned int>(casterID + vnum + 12345));
}

void PhantasmInfo::learnPhantasm(int casterID, int vnum, int bonusDuration)
{
    // Get the index
    const MOB_INDEX_DATA * mobIndex(lookupMobIndex(vnum));
    if (mobIndex == NULL)
        return;

    // Add the info
    m_phantasms.push_back(Info());
    Info & info(m_phantasms[m_phantasms.size() - 1]);
    info.vnum = vnum;
    info.promotions = 0;

    // Determine the set of possible free traits, either Natural or Normal (anything less likely not included)
    std::vector<PhantasmTrait::Trait> naturals;
    std::vector<PhantasmTrait::Trait> normals;
    for (unsigned int i(0); i < PhantasmTrait::Max; ++i)
    {
        PhantasmTrait::Trait trait(static_cast<PhantasmTrait::Trait>(i));
        switch (PhantasmTrait::AffinityFor(trait, *mobIndex, info.traits, true))
        {
            case PhantasmTrait::Natural: naturals.push_back(trait); break;
            case PhantasmTrait::Normal: normals.push_back(trait); break;
            default: break;
        }
    }
    size_t naturalsCount(naturals.size());
    naturals.insert(naturals.end(), normals.begin(), normals.end());

    // Determine how many random free traits to assign
    // (bonusDuration)% chance for every level over 50; 
    // Badass aggressive mobs double the odds
    // More time spent learning the mob contributes, as well
    int chanceOfFreebie((mobIndex->level - 50) * (3 + UMIN(2, bonusDuration)));
    if (IS_SET(mobIndex->act, ACT_BADASS) && IS_SET(mobIndex->act, ACT_AGGRESSIVE))
        chanceOfFreebie *= 2;

    // Randomize according to caster and mob
    srand(makeSeed(casterID, vnum));
    for (unsigned int i(0); i < 3 && !naturals.empty(); ++i)
    {
        if (rand() % 100 > chanceOfFreebie)
            break;

        // Add a freebie chosen from the set of possibles, favoring the naturals
        size_t index;
        if (naturalsCount > 0 && rand() % 10 <= 3)
            index = rand() % naturalsCount;
        else
            index = rand() % naturals.size();

        // Add in the freebie, then update the invariants and loop
        info.traits.push_back(PhantasmTrait::TakenInfo(naturals[index]));
        if (index < naturalsCount)
            --naturalsCount; 

        naturals.erase(naturals.begin() + index);
        chanceOfFreebie /= 2;
    }
    srand(time(0));
}

void PhantasmInfo::forgetPhantasm(CHAR_DATA & ch, CHAR_DATA & victim, unsigned int index)
{
    // Get the index
    const MOB_INDEX_DATA * mobIndex(lookupMobIndex(m_phantasms[index].vnum));
    if (mobIndex == NULL)
    {
        send_to_char("An error has occurred; please contact the Immortals.\n", &ch);
        return;
    }

    // Echo the forgetting
    std::ostringstream mess;
    mess << "You relax your mind, letting all knowledge of how to weave an illusion of " << mobIndex->short_descr << " slip away.\n";
    send_to_char(mess.str().c_str(), &victim);

    if (&ch != &victim)
    {
        mess.str("");
        mess << "You make $N forget how to weave an illusion of " << mobIndex->short_descr << ".";
        act(mess.str().c_str(), &ch, NULL, &victim, TO_CHAR);
    }

    // Remove the phantasm
    m_phantasms.erase(m_phantasms.begin() + index);
}

bool PhantasmInfo::isPhantasmHere(const ROOM_INDEX_DATA & room, int vnum)
{
    for (CHAR_DATA * ch(room.people); ch != NULL; ch = ch->next_in_room)
    {
        if (IS_NPC(ch) && ch->pIndexData->vnum == MOB_VNUM_PHANTASMALMIRROR && ch->mobvalue[MobSlotInfoIndex] == vnum)
            return true;
    }

    return false;
}

bool PhantasmInfo::isPhantasmHere(const ROOM_INDEX_DATA & room, unsigned int index) const
{
    return isPhantasmHere(room, m_phantasms[index].vnum);
}

void PhantasmInfo::checkReduceCooldown(const CHAR_DATA & ch)
{
    // Check disqualifiers
    if (!IS_NPC(&ch) || ch.pIndexData->vnum != MOB_VNUM_PHANTASMALMIRROR || ch.master == NULL)
        return;

    // Find the cooldown
    for (AFFECT_DATA * paf(get_affect(ch.master, gsn_empowerphantasm)); paf != NULL; paf = get_affect(ch.master, gsn_empowerphantasm, paf))
    {
        if (paf->duration > 1 && paf->modifier == ch.mobvalue[MobSlotInfoIndex])
        {
            --paf->duration;
            return;
        }
    }
}

CHAR_DATA * PhantasmInfo::generatePhantasm(int casterLevel, unsigned int index) const
{
    const Info & info(m_phantasms[index]);

    // Generate the base mobile
    const MOB_INDEX_DATA * mobIndex(lookupMobIndex(info.vnum));
    if (mobIndex == NULL)
        return NULL;

    CHAR_DATA * mob(create_mobile(get_mob_index(MOB_VNUM_PHANTASMALMIRROR)));
    if (mob == NULL)
    {
        bug("generatePhantasm failed due to bad mob creation", 0);
        return NULL;
    }

    // Mark the vnum whence this mob came
    mob->mobvalue[MobSlotInfoIndex] = info.vnum;

    // Copy over relevant parts of the mob from the index
    setName(*mob, mobIndex->player_name);
    copyMobString(mob->short_descr, mobIndex->short_descr);
    copyMobString(mob->long_descr, mobIndex->long_descr);
    copyMobString(mob->description, mobIndex->description);
    setFakeName(*mob, mobIndex->player_name);
    copyMobString(mob->dam_verb, mobIndex->dam_verb);
    mob->level = UMIN(mobIndex->level, casterLevel);
    mob->race = mobIndex->race;
    mob->size = mobIndex->size;
    mob->max_hit = 1000;
    mob->vuln_flags = mobIndex->vuln_flags;
    mob->res_flags = mobIndex->res_flags | mobIndex->imm_flags;
    mob->imm_flags = mobIndex->imm_flags & (IMM_BLIND | IMM_SUMMON | IMM_CHARM);

    for (unsigned int i(0); i < MAX_STATS; ++i)
        mob->perm_stat[i] = 17 + (mob->level / 10);

    for (unsigned int i(0); i < MAX_RESIST; ++i)
        mob->resist[i] = URANGE(-20, mobIndex->resist[i], 20);

    if (mobIndex->dam_type > 0) mob->dam_type = mobIndex->dam_type;
    if (mobIndex->sex != 3) mob->sex = mobIndex->sex;

    switch (mobIndex->alignment)
    {
        case ALIGN_GOOD:    mob->alignment = 750;   break;
        case ALIGN_NEUTRAL: mob->alignment = 0;     break;
        case ALIGN_EVIL:    mob->alignment = -750;  break;
    }

    // Now determine the mob's base power according to level
    mob->hitroll = mob->level / 3;
    mob->damage[0] = 1;
    mob->damage[1] = mob->level;
    mob->damage[2] = 0;

    // Perform trait-based adjustments
    for (size_t i(0); i < info.traits.size(); ++i)
        adjustPhantasm(*mob, info.traits[i].trait, info.traits[i].count);

    // Restore the mob
    mob->hit = mob->max_hit;
    mob->mana = mob->max_mana;
    mob->damroll = mob->damage[2];
    return mob;
}

int PhantasmInfo::drainFor(PhantasmTrait::Trait trait, PhantasmTrait::Affinity affinity)
{
    // Start with the trait's normal drain
    int result(PhantasmTrait::DrainFor(trait));

    // Adjust by affinity
    switch (affinity)
    {
        case PhantasmTrait::Normal: break;
        case PhantasmTrait::Natural: --result; break;
        case PhantasmTrait::Unlikely: result += 2; break;
        case PhantasmTrait::Absurd:
            bug("Absurd phantasm trait detected in drainFor", 0);
            break;
    }

    return result;
}

int PhantasmInfo::totalDrainFor(const Info & info, const MOB_INDEX_DATA & mobIndex)
{
    // Adjust for each trait
    int result(BaseDrain);
    for (size_t i(0); i < info.traits.size(); ++i)
        result += (drainFor(info.traits[i].trait, PhantasmTrait::AffinityFor(info.traits[i].trait, mobIndex, info.traits, false)) * info.traits[i].count);

    // Return at least the minimum drain
    return UMAX(result, MinimumDrain);
}

int PhantasmInfo::totalDrainFor(const CHAR_DATA & ch)
{
    // Lookup the info
    const Info * info(lookupInfo(ch));
    if (info == NULL)
        return 0;

    // Lookup the mob index
    const MOB_INDEX_DATA * mobIndex(lookupMobIndex(info->vnum));
    if (mobIndex == NULL)
        return 0;

    // Return the drain
    return totalDrainFor(*info, *mobIndex);
}

unsigned int PhantasmInfo::calculateNextCooldown(unsigned int seed, unsigned int traitSeed, unsigned int promotions, PhantasmTrait::Affinity affinity)
{
    // Cooldown is 3 + (1.7 ^ (promotions - 1)) + random(0, 6) real hours
    unsigned int result(3);
    float base(1.7f);
    
    // Adjust for affinity
    switch (affinity)
    {
        case PhantasmTrait::Natural: result -= 2; base -= 0.1f; break;
        case PhantasmTrait::Normal: break;
        case PhantasmTrait::Unlikely: result += 10; break;
        default: bug("Unexpected affinity in phantasm cooldown calculation", 0); break;
    }

    if (promotions > 0)
        result += static_cast<unsigned int>(std::pow(base, static_cast<float>(promotions - 1)));


    // Convert to ticks and add a random factor of 0 - 6 real hours (but in ticks, for more randomness)
    result *= 120;
    srand(seed + traitSeed);
    result += (rand() % (6 * 120));
    srand(time(0));

    // Cap all cooldowns
    return UMIN(result, MaxCooldown);
}

std::string PhantasmInfo::listPossibleEmpowerments(int casterID, unsigned int index) const
{
    static const size_t DrainWidth(5);
    static const size_t HoursWidth(8);
    
    // Lookup the mob
    const Info & info(m_phantasms[index]);
    const MOB_INDEX_DATA * mobIndex(lookupMobIndex(info.vnum));
    if (mobIndex == NULL)
        return "";

    // Prepare the stream and write the header
    std::ostringstream mess;
    mess << std::setfill(' ') << std::left;
    mess << "Available Empowerments for " << mobIndex->short_descr << "\n";
    mess << "---------------------------";

    for (size_t i(0); mobIndex->short_descr[i] != '\0'; ++i)
        mess << "-";

    mess << "\n\n";
    mess << std::setw(TraitNameMaxWidth + 1) << "Name" << std::right << std::setw(DrainWidth) << "Drain" << ' ';
    mess << std::setw(HoursWidth) << "Cooldown" << ' ' << std::left << "Description\n";
    mess << std::setw(TraitNameMaxWidth + 1) << "----" << std::right << std::setw(DrainWidth) << "-----" << ' ' ;
    mess << std::setw(HoursWidth) << "--------" << ' ' << std::left << "-----------\n";

    // Build the display
    unsigned int seed(makeSeed(casterID, info.vnum));
    for (unsigned int i(0); i < PhantasmTrait::Max; ++i)
    {
        // Get the affinity and skip any which come up absurd
        PhantasmTrait::Trait trait(static_cast<PhantasmTrait::Trait>(i));
        PhantasmTrait::Affinity affinity(PhantasmTrait::AffinityFor(trait, *mobIndex, info.traits, true));
        if (affinity == PhantasmTrait::Absurd)
            continue;

        // Determine cooldown string
        int cooldownTicks(calculateNextCooldown(seed, trait, info.promotions, affinity));
        std::ostringstream cooldownStr;
        cooldownStr << (cooldownTicks / 2) << "." << (cooldownTicks % 2 == 0 ? '0' : '5');

        // List the trait name, drain, cooldown, description
        mess << std::setw(TraitNameMaxWidth + 1) << PhantasmTrait::NameFor(trait) << std::right;
        mess << std::setw(DrainWidth) << drainFor(trait, affinity) << ' ';
        mess << std::setw(HoursWidth) << cooldownStr.str() << ' ';
        mess << std::left << PhantasmTrait::DescriptionFor(trait) << '\n';
    }

    return mess.str();
}

void PhantasmInfo::listPhantasm(std::ostream & mess, size_t index) const
{
    // Lookup the mob
    const Info & info(m_phantasms[index]);
    const MOB_INDEX_DATA * mobIndex(lookupMobIndex(info.vnum));
    if (mobIndex == NULL)
        return;
 
    // Write the number, drain, and short desc
    mess << '[' << std::setw(1) << (index + 1) << "] ";
    mess << std::right << std::setw(5) << totalDrainFor(info, *mobIndex) << ' ';
    mess << mobIndex->short_descr;
    
    if (!info.traits.empty())
        mess << ": {W";

    // Write the traits
    for (size_t i(0); i < info.traits.size(); ++i)
    {
        if (i != 0)
            mess << ", ";

        // Determine the trait name, with any appropriate roman numerals
        mess << PhantasmTrait::NameFor(info.traits[i].trait);
        if (info.traits[i].count > 1)
            mess << ' ' << RomanNumerals::convertTo(info.traits[i].count);
    }

    // Finish with a newline
    mess << "{x\n";
}

std::string PhantasmInfo::listPhantasms() const
{
    std::ostringstream mess;
    mess << std::setfill(' ');
    mess << " #  Drain Name: {WEmpowerments{x\n";
    mess << "--- ----- ------------------\n";

    // Write each phantasm in turn
    for (size_t i(0); i < m_phantasms.size(); ++i)
        listPhantasm(mess, i);

    return mess.str();
}

void PhantasmInfo::adjustPhantasm(CHAR_DATA & mob, PhantasmTrait::Trait trait, unsigned int count)
{
    // Adjust for the traits which affect initial power
    switch (trait)
    {
        case PhantasmTrait::Overwhelming:   mob.level += (count * 2);       break;
        case PhantasmTrait::Precise:        mob.hitroll += (count * 15);    break;
        case PhantasmTrait::Strong:         mob.damage[2] += (count * 10);  break;
        case PhantasmTrait::Mighty:         mob.damage[2] += (count * 16);  break;
        case PhantasmTrait::Legendary:      mob.damage[2] += (count * 25);  break;
        default: break;
    }
}
