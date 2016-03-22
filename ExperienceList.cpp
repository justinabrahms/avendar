#include "ExperienceList.h"
#include "demons.h"
#include "Oaths.h"
#include <sstream>

ExperienceList::Group::Group() : amount(0) {}
ExperienceList::Info::Info(long idIn, int amountIn) : id(idIn), amount(amountIn), lastHit(current_time) {}

void ExperienceList::OnDamage(const CHAR_DATA & ch, int amount)
{
    Clean();

    // Check for gravebeat
    long id(ch.id);
    AFFECT_DATA * paf(get_affect(&ch, gsn_gravebeat));
    if (paf != NULL && paf->modifier != 0)
        id = paf->modifier;

    for (size_t i(0); i < m_infos.size(); ++i)
    {
        // Look for a match
        if (m_infos[i].id == id)
        {
            // Found a match, so update the values
            m_infos[i].amount += amount;
            m_infos[i].lastHit = current_time;
            return;
        }
    }

    // No match found, so add one in
    m_infos.push_back(Info(id, amount));
}

void ExperienceList::OnDeath(const CHAR_DATA & victim)
{
    // Must be in a room for this to count
    if (victim.in_room == NULL)
        return;

    // Resolve the people into groups
    Clean();
    std::vector<Group> groups;
    for (CHAR_DATA * ch(victim.in_room->people); ch != NULL; ch = ch->next_in_room)
    {
        bool foundGroup(false);
        for (size_t i(0); i < groups.size(); ++i)
        {
            // Guaranteed to have at least one person in each group
            if (is_same_group(groups[i].people[0], ch))
            {
                AddToGroup(groups[i], *ch);
                foundGroup = true;
                break;
            }
        }

        // If no group found, make a new one
        if (!foundGroup)
        {
            groups.push_back(Group());
            AddToGroup(groups[groups.size() - 1], *ch);
        }
    }

    // Groups have been resolved; sum up the total group damage contribution
    // Sum from the infos rather than the groups to avoid heros fleeing at the last second to give a
    // huge bonus to their low-level groupmates
    int totalAmount(0);
    for (size_t i(0); i < m_infos.size(); ++i)
        totalAmount += m_infos[i].amount;

    // Iterate the groups, computing each individual's XP
    for (size_t i(0); i < groups.size(); ++i)
    {
        // Compute the sum of the squares of the group's levels, as well as the PC-only size
        int groupLevels(0);
        unsigned int groupSize(0);
        for (size_t j(0); j < groups[i].people.size(); ++j)
        {
            CHAR_DATA * ch(groups[i].people[j]);
            groupLevels += (ch->level * ch->level);
            if (!IS_NPC(ch))
                ++groupSize;
        }
        int averageLevels(groupLevels / groups[i].people.size());
        groupSize = UMAX(groupSize, 1);

        // Iterate the individuals within the group
        for (size_t j(0); j < groups[i].people.size(); ++j)
        {
            // Skip any heros or npcs
            CHAR_DATA * ch(groups[i].people[j]);
            if (IS_HERO(ch) || IS_NPC(ch))
                continue;

            // Compute this individual's base XP
            int xp(ComputeXP(*ch, victim, groupSize));
            if (totalAmount <= 0 || averageLevels <= 0)
                xp = 0;
            else
            {
                // Adjust for group's contribution
                xp = (xp * groups[i].amount) / totalAmount;

                // Adjust for individual's contribution within the group, according to squared levels
                int squaredLevel(ch->level * ch->level);
                if (squaredLevel < averageLevels)
                {
                    // Adjust for oaths
                    CHAR_DATA * oathHolder(Oaths::OathHolderFor(*ch));
                    if (oathHolder != NULL && oathHolder->in_room == ch->in_room)
                        squaredLevel += (averageLevels - squaredLevel) / 5;

                    // Calculate the xp
                    xp = (xp * squaredLevel) / averageLevels;
                }
               
                // Apply random factor
                xp = number_range(xp * 3, xp * 5) / 4;
            }

            // Notify and award the xp
            std::ostringstream mess;
            mess << "You receive " << xp << " experience point" << (xp == 1 ? "" : "s") << '\n';
            send_to_char(mess.str().c_str(), ch);
            gain_exp(ch, xp);
        }
    }
}

void ExperienceList::Clean()
{
    size_t i(0);
    while (i < m_infos.size())
    {
        // Check whether this info is expired
        if ((current_time - m_infos[i].lastHit) < DeleteTimeSeconds)
            ++i;
        else
        {
            // Expired, so remove it
            m_infos[i] = m_infos[m_infos.size() - 1];
            m_infos.pop_back();
        }
    }
}

void ExperienceList::AddToGroup(Group & group, CHAR_DATA & ch) const
{
    // Add this person to the group
    group.people.push_back(&ch);

    // Add in this person's damage contribution to the group
    for (size_t i(0); i < m_infos.size(); ++i)
    {
        if (ch.id == m_infos[i].id)
        {
            group.amount += m_infos[i].amount;
            break;
        }
    }
}

int ExperienceList::ComputeXP(const CHAR_DATA & ch, const CHAR_DATA & victim, unsigned int groupSize)
{
    // Check disqualifiers
    if (IS_NPC(&victim) 
    && (IS_SET(victim.act, ACT_ILLUSION) || victim.pIndexData->vnum == MOB_VNUM_SNAKE
    || (victim.pIndexData->vnum >= MOB_VNUM_DEMON_FIRST && victim.pIndexData->vnum <= MOB_VNUM_DEMON_LAST)
    || strstr(victim.name, "rangercall")))
        return 0;

    // Compute base XP, determined by level gap
    int baseXP(0);
    int levelRange(victim.level - ch.level);
    switch (levelRange)
    {
        case -9 :	baseXP =   1;	break;
        case -8 :	baseXP =   2;	break;
        case -7 :	baseXP =   5;	break;
        case -6 : 	baseXP =   9;	break;
        case -5 :	baseXP =  11;	break;
        case -4 :	baseXP =  22;	break;
        case -3 :	baseXP =  33;	break;
        case -2 :	baseXP =  50;	break;
        case -1 :	baseXP =  66;	break;
        case  0 :	baseXP =  83;	break;
        case  1 :	baseXP =  99;	break;
        case  2 :	baseXP = 121;	break;
        case  3 :	baseXP = 143;	break;
        case  4 :	baseXP = 165;	break;
        default:
            if (levelRange > 4)
                baseXP = 160 + 20 * (levelRange - 4);
            break;
    } 
    
    // Modify according to alignment
    if (!IS_NPC(&victim) || !IS_SET(victim.act, ACT_NOALIGN))
    {
        if (IS_GOOD(&ch))
        {
            // Goodies get bonuses for killing evils, and penalties for killing goodies
            if (victim.alignment <= -750)       baseXP = (baseXP * 130) / 100;
            else if (victim.alignment < -500)   baseXP = (baseXP * 120) / 100;
            else if (victim.alignment > 0)      baseXP = -30; 
        }
        else if (IS_EVIL(&ch))                  baseXP = (baseXP * 120) / 100;  // Evils get decent bonuses for killing anyone
        else                                    baseXP = (baseXP * 110) / 100;  // Neutrals get small bonuses for killing anyone
    }

    // Bonus XP to low levels, reduced XP at high levels
    if (ch.level < 11) baseXP = (10 * baseXP) / (UMIN(ch.level, 5) + 4);
    else if (ch.level > 40)
    	baseXP = (15 * baseXP) / (ch.level - 25);

    // Adjust for group size
    if (groupSize <= 3) baseXP *= 2;
    else baseXP = (baseXP * 6) / groupSize;

    // Adjust for badass and hardcore
    if (IS_NPC(&victim) && IS_SET(victim.act, ACT_BADASS)) baseXP *= 8;
    if (IS_SET(ch.nact, PLR_HARDCORE)) baseXP *= HC_MULT;
    return baseXP;
}
