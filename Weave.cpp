#include "Weave.h"
#include "tables.h"
#include "spells_spirit.h"
#include "Runes.h"
#include <map>
#include <sstream>
#include <iomanip>
#include <fstream>

std::set<ROOM_INDEX_DATA *> Weave::s_changedRooms;

void Weave::SaveWeave()
{
    // Open the output file
    std::ofstream fout(WEAVE_FILE);
    if (!fout.is_open())
    {
        bug("Unable to open weave file for writing", 0);
        return;
    }

    // Iterate the rooms
    for (unsigned int i(0); i < MAX_KEY_HASH; ++i)
    {
        for (const ROOM_INDEX_DATA * room(room_index_hash[i]); room != NULL; room = room->next)
        {
            // Ignore fountless rooms
            if (room->ley_group == NULL || !room->ley_group->HasFount())
                continue;

            // Room has a fount, so save it off
            fout << room->vnum << ' ' << room->ley_group->FountPositivePower() << ' ';
            fout << room->ley_group->FountOrderPower() << ' ' << room->ley_group->AttunedID() << '\n';
        }
    }
}

bool Weave::LoadWeave()
{
    // Open the input file
    std::ifstream fin(WEAVE_FILE);
    if (!fin.is_open())
    {
        bug("Unable to open weave file for reading", 0);
        return false;
    }

    // Read in the founts
    while (!fin.eof())
    {
        // Read in the values
        int vnum;               fin >> vnum;
        int positivePower;      fin >> positivePower;
        int orderPower;         fin >> orderPower;
        long attunedID;         fin >> attunedID;

        // Basic error checking
        if (fin.fail() && !fin.eof())
        {
            bug("Failed to fully read weave file", 0);
            return false;
        }

        // Look up the room
        ROOM_INDEX_DATA * room(get_room_index(vnum));
        if (room == NULL)
        {
            bug("Missing room referenced in weave file [vnum: %d]", vnum);
            continue;
        }

        // Build the ley info
        positivePower = URANGE(Fount::PowerMin, positivePower, Fount::PowerMax);
        orderPower = URANGE(Fount::PowerMin, orderPower, Fount::PowerMax);
        LeyInfo leyInfo(room, orderPower, positivePower);

        // Set up the ley group
        if (room->ley_group == NULL) room->ley_group = new LeyGroup(leyInfo);
        else room->ley_group->SetFount(leyInfo);
        room->ley_group->SetAttunedID(attunedID);
    }
    return true;
}

void Weave::UpdateAttunementsFor(CHAR_DATA & ch, int count)
{
    // Determine the modifier from the count
    static const int BaseValue = 15;
    int modifier(0);
  
    // Only allow non-zero modifiers if the skill is actually possessed
    if (get_skill(&ch, gsn_attunefount) > 0)
    {
        if (count > BaseValue)
        {
            // Anything over 15 founts is only worth a single point
            modifier = (count - BaseValue);
            count = BaseValue;
        }

        // This is a reduced formula using summations so it may not be clear what is going on:
        // Basically, the first fount is worth 2 * BaseValue, the next is worth 2 * (BaseValue - 1), the next 2 * (BaseValue - 2), and so on, until eventually they are worth exactly 1 each
        // So for BV = 15, the values are 30, 28, 26, 24, ..., 6, 4, 2, 1, 1, 1, ...
        modifier += (2 * ((BaseValue * count) - ((count * (count - 1)) / 2)));
    }

    // Compare the new modifier to the existing to see whether there is any change
    int prevModifier(0);
    AFFECT_DATA * paf(get_affect(&ch, gsn_attunefount));
    if (paf != NULL) prevModifier = paf->modifier;
    if (prevModifier == modifier)
        return;

    // Character needs a change; strip any existing effect and add the new one if appropriate
    affect_strip(&ch, gsn_attunefount);
    if (modifier == 0)
        send_to_char("You feel the last of your attunements leave you.\n", &ch);
    else
    {
        // Prepare the effect
        AFFECT_DATA af = {0};
        af.where    = TO_AFFECTS;
        af.type     = gsn_attunefount;
        af.duration = -1;
        af.modifier = modifier;
        af.location = APPLY_HIT;
        affect_to_char(&ch, &af);
    
        af.location = APPLY_MANA;
        affect_to_char(&ch, &af);

        // Adjust for gains (ignore losses)
        if (prevModifier < modifier)
        {
            ch.hit += (modifier - prevModifier);
            ch.mana += (modifier - prevModifier);
        }

        // Send an echo
        send_to_char("You feel the power of your attunements shift.\n", &ch);
    }
}

void Weave::UpdateAttunements()
{
    // Build a map of attunement counts by id
    std::map<long, unsigned int> attunements;
    for (unsigned int i(0); i < MAX_KEY_HASH; ++i)
    {
        for (const ROOM_INDEX_DATA * roomIter(room_index_hash[i]); roomIter != NULL; roomIter = roomIter->next)
        {
            // Check whether this room has an attuned fount
            if (roomIter->ley_group != NULL && roomIter->ley_group->AttunedID() != -1)
            {
                // Start an attunement count if none present, or increment if already present
                std::map<long, unsigned int>::iterator iter(attunements.find(roomIter->ley_group->AttunedID()));
                if (iter == attunements.end())
                    attunements.insert(std::make_pair(roomIter->ley_group->AttunedID(), 1));
                else
                    ++iter->second;
            }
        }
    }

    // Now iterate the characters, looking for attunements
    for (CHAR_DATA * ch(char_list); ch != NULL; ch = ch->next)
    {
        // Determine the attunement count
        int count(0);
        std::map<long, unsigned int>::const_iterator iter(attunements.find(ch->id));
        if (iter != attunements.end())
            count = static_cast<int>(iter->second);

        // Adjust attunements
        UpdateAttunementsFor(*ch, count);
        Runes::UpdateAttunementsFor(*ch, count);
   }
}

bool Weave::HasWeave(const ROOM_INDEX_DATA & room)
{
    if (room.ley_group == NULL || area_is_affected(room.area, gsn_sunderweave))
        return false;

    return (room.ley_group->HasFount() || room.ley_group->LineCount() > 0);
}

int Weave::AbsolutePositivePower(const ROOM_INDEX_DATA & room)
{
    // If no ley here or the area is sundered, just return 0
    if (room.ley_group == NULL || area_is_affected(room.area, gsn_sunderweave)) 
        return 0;

    // Sum the founts (which never get the modifiers) with the modified lines
    int result(room.ley_group->FountPositivePower());
    for (unsigned int i(0); i < room.ley_group->LineCount(); ++i)
        result += ApplyPositiveModifiers(room.ley_group->Line(i).PositivePower, room);

    return result;
}

int Weave::AbsoluteOrderPower(const ROOM_INDEX_DATA & room)
{
    // If not ley here or the area is sundered, just return 0
    if (room.ley_group == NULL || area_is_affected(room.area, gsn_sunderweave))
        return 0;

    // Sum the founts (which never get the modifiers) with the modified lines
    int result(room.ley_group->FountOrderPower());
    for (unsigned int i(0); i < room.ley_group->LineCount(); ++i)
        result += ApplyOrderModifiers(room.ley_group->Line(i).OrderPower, room);

    return result;
}

int Weave::ApplyPositiveModifiers(int positivePower, const ROOM_INDEX_DATA & room)
{
    int boosts(0);

    // Room modifier boosts
    if (room_is_affected(const_cast<ROOM_INDEX_DATA*>(&room), gsn_sanctify)) ++boosts;
    if (room_is_affected(const_cast<ROOM_INDEX_DATA*>(&room), gsn_desecration)) --boosts;

    // Seasonal boosts; summer is more positive, winter is less positive
    if (!str_cmp(season_table[time_info.season].name, "summer")) ++boosts;
    else if (!str_cmp(season_table[time_info.season].name, "winter")) --boosts;

    // Boosts cannot make negative lines positive nor positive lines negative
    if (positivePower < 0) positivePower = URANGE(Fount::PowerMin, positivePower + boosts, 0);
    else if (positivePower > 0) positivePower = URANGE(0, positivePower + boosts, Fount::PowerMax);
    else positivePower = URANGE(Fount::PowerMin, positivePower + boosts, Fount::PowerMax);
    return positivePower;
}

int Weave::ApplyOrderModifiers(int orderPower, const ROOM_INDEX_DATA & room)
{
    int boosts(0);

    // Storms of sufficient power with lightning increase the chaotic power
    if ((room.area->w_cur.storm_str >= 35 && room.area->w_cur.precip_type != 0 && room.area->w_cur.precip_type != 1 && room.area->w_cur.lightning_str > 10)
    || area_is_affected(room.area, gsn_lightning_storm))
        --boosts;

    // Gravity wells increase order power
    if (area_is_affected(room.area, gsn_gravitywell))
        ++boosts;

    // Seasonal boosts; autumn is more ordered, spring is less ordered
    if (!str_cmp(season_table[time_info.season].name, "autumn")) ++boosts;
    else if (!str_cmp(season_table[time_info.season].name, "spring")) --boosts;

    // Boosts cannot make order lines chaotic nor chaos lines ordered
    if (orderPower < 0) orderPower = URANGE(Fount::PowerMin, orderPower + boosts, 0);
    else if (orderPower > 0) orderPower = URANGE(0, orderPower + boosts, Fount::PowerMax);
    else orderPower = URANGE(Fount::PowerMin, orderPower + boosts, Fount::PowerMax);
    return orderPower;
}

void Weave::ShowWeave(CHAR_DATA & ch, const ROOM_INDEX_DATA & room)
{
    // Check for skill
    bool hasGraven(is_affected(&ch, gsn_gravenmind));
    bool gravenOnly(false);
    if (number_percent() > get_modifier(ch.affected, gsn_weavesense) && !is_an_avatar(&ch))
    {
        if (hasGraven) gravenOnly = true;
        else return;
    }

    // Check for sundered weave
    if (area_is_affected(room.area, gsn_sunderweave))
        return;

    // Check for a fount
    check_improve(&ch, NULL, gsn_weavesense, TRUE, 4);
    std::ostringstream mess;
    mess << std::hex;
    bool anyWeave(false);
    if (room.ley_group != NULL)
    {
        if (room.ley_group->HasFount() && (!gravenOnly || room.ley_group->FountOrderPower() > 0))
        {
            anyWeave = true;
            long attunedID(room.ley_group->AttunedID());
            if (attunedID != LeyGroup::Unattuned)
                mess << ((attunedID == ch.id) ? "[{WResonant{x] " : "[{WDissonant{x] ");
            
            if (gravenOnly)
               mess << "A sense of deep, humming power fills this place, marking it as a nexus of order.";
            else
            {
                // Note that founts don't get modified by normal ley modifiers
                mess << GenerateFountDescription(room.ley_group->FountOrderPower(), room.ley_group->FountPositivePower(), AdjacentLeyRooms(room, &room));
                if (IS_IMMORTAL((&ch)))
                    mess << " {c[ID: " << reinterpret_cast<unsigned long>(&room) << "]{x";
            }
            mess << "\n";
        }

        // Check for ley lines
        if (!gravenOnly)
        {
            for (unsigned int i(0); i < room.ley_group->LineCount(); ++i)
            {
                anyWeave = true;
                const LeyInfo & leyLine(room.ley_group->Line(i));
                mess << GenerateLineDescription(ApplyOrderModifiers(leyLine.OrderPower, room), ApplyPositiveModifiers(leyLine.PositivePower, room), AdjacentLeyRooms(room, leyLine.ID));
                if (IS_IMMORTAL((&ch)))
                    mess << " {c[ID: " << reinterpret_cast<unsigned long>(leyLine.ID) << "]{x";
                mess << "\n";
            }
        }
    }

    if (hasGraven)
    {
        // Graven mind can sense order founts in adjacent rooms
        for (unsigned int i(0); i < Direction::Max; ++i)
        {
            Direction::Value direction(static_cast<Direction::Value>(i));
            ROOM_INDEX_DATA * nextRoom(Direction::Adjacent(room, direction));
            if (nextRoom != NULL && nextRoom->ley_group != NULL && nextRoom->ley_group->HasFount() && nextRoom->ley_group->FountOrderPower() > 0)
            {
                anyWeave = true;
                mess << "The deep, steady thrum of an order nexus resonates " << Direction::SourceNameFor(direction) << ".\n";
            }
        }
    }

    if (anyWeave)
    {
        mess << "\n";
        send_to_char(mess.str().c_str(), &ch);
    }
}

void Weave::ShowNew(const ROOM_INDEX_DATA & room)
{
    for (CHAR_DATA * ch(room.people); ch != NULL; ch = ch->next_in_room)
    {
        // Check for skill
        if (number_percent() >= get_modifier(ch->affected, gsn_weavesense) && !is_an_avatar(ch))
            continue;

        // Skill check passed
        check_improve(ch, NULL, gsn_weavesense, TRUE, 4);
        send_to_char("You sense the Weave twist and flex around you, its primal energy branching and flowing in new patterns!\n", ch);
    }
}

std::vector<Direction::Value> Weave::AdjacentLeyRooms(const ROOM_INDEX_DATA & room, const void * id)
{
    // Iterate the rooms coming from this one
    std::vector<Direction::Value> result;
    for (unsigned int i(0); i < Direction::Max; ++i)
    {
        // Check whether there is a valid room in this direction which matches this ley id
        Direction::Value value(static_cast<Direction::Value>(i));
        ROOM_INDEX_DATA * adjacent(Direction::Adjacent(room, value));
        if (adjacent != NULL && adjacent->ley_group != NULL 
        && ((adjacent->ley_group->HasFount() && adjacent == id) || adjacent->ley_group->LineByID(id) != LeyGroup::NotFound))
            result.push_back(value);
    }
    return result;
}

std::string Weave::GenerateFountDescription(int orderPower, int positivePower, const std::vector<Direction::Value> & directions)
{
    std::ostringstream mess;
    mess << "A fount of ";
    
    // Factor in positive power
    bool addedWords(true);
    switch (positivePower)
    {
        case 2: mess << "intensely positive "; break;
        case 1: mess << "positive "; break;
        case 0: addedWords = false; break;
        case -1: mess << "negative "; break;
        case -2: mess << "intensely negative "; break;
        default:
            bug("Invalid positive power in fount description", 0);
            addedWords = false;
            break;
    }

    // Factor in order power
    const char * newWords(NULL);
    switch (orderPower)
    {
        case 2: newWords = "crystalline "; break;
        case 1: newWords = "ordered "; break;
        case 0: break;
        case -1: newWords = "chaotic "; break;
        case -2: newWords = "crackling chaotic "; break;
        default: bug("Invalid order power in fount description", 0); break;
    }

    if (newWords != NULL)
    {
        if (addedWords)
            mess << "and ";

        mess << newWords;
    }

    // Factor in direction
    mess << "energy wells up here";
    switch (directions.size())
    {
        case 0: break;
        case 1: mess << ", flowing " << DirectionString(directions[0], false); break;
        case 2: mess << ", flowing " << DirectionString(directions[0], false) << " and " << DirectionString(directions[1], true); break;
        default:
            mess << ", branching " << DirectionString(directions[0], false);
            for (size_t i(1); i + 1 < directions.size(); ++i)
                mess << ", " << DirectionString(directions[i], true);
            mess << " and " << DirectionString(directions[directions.size() - 1], true);
            break;
    }

    mess << ".";
    return mess.str();
}

std::string Weave::GenerateLineDescription(int orderPower, int positivePower, const std::vector<Direction::Value> & directions)
{
    std::ostringstream mess;

    // Build the color of the ley line based on the positive power
    switch (positivePower)
    {
        case 2: mess << "An intense band of golden"; break; 
        case 1: mess << "A shimmering band of golden"; break;
        case 0: mess << "A pale band of"; break;
        case -1: mess << "A dusky band of negative"; break;
        case -2: mess << "An intense band of negative"; break;
        default: 
            bug("Invalid positive power in ley line description", 0);
            mess << "A band of";
            break;
    }

    // Build the directions; change the sentence structure based on how many directions branch off here
    mess << " energy ";
    switch (directions.size())
    {
        case 0: mess << "flows here"; break;
        case 1: mess << "flows " << DirectionString(directions[0], false); break;
    
        case 2: 
            if (directions[0] == Direction::ReverseOf(directions[1])) mess << "winds";
            else mess << "bends";
            mess << " " << DirectionString(directions[0], false) << " and " << DirectionString(directions[1], true); 
            break;
    
        default:
            mess << "branches " << DirectionString(directions[0], false);
            for (size_t i(1); i + 1 < directions.size(); ++i)
                mess << ", " << DirectionString(directions[i], true);
            mess << " and " << DirectionString(directions[directions.size() - 1], true);
            break;
    }

    // Now add the rest of the of the description based on order power
    switch (orderPower)
    {
        case 2: mess << ", throbbing with a potent, steady rhythm."; break;
        case 1: mess << ", humming steadily."; break;
        case 0: mess << "."; break;
        case -1: mess << ", pulsing erratically."; break;
        case -2: mess << ", crackling with chaotic power."; break;
        default:
            bug("Invalid order power in ley line description", 0);
            mess << "."; 
            break;
    }
        
    return mess.str();
}

std::string Weave::DirectionString(Direction::Value value, bool raw)
{
    const char * rawValue(Direction::NameFor(value));
    const char * prefix("to the ");
    switch (value)
    {
        case Direction::Up: prefix = "";    rawValue = "upwards";   break;
        case Direction::Down: prefix = "";  rawValue = "downwards"; break;
        default: break;
    }

    return (raw ? std::string(rawValue) : (std::string(prefix) + rawValue));
}

void Weave::Regenerate()
{
    // Regenerate the Weave
    s_changedRooms.clear();
    for (AREA_DATA * area(area_first); area != NULL; area = area->next)
        Regenerate(*area);

    // Echo about changes
    for (std::set<ROOM_INDEX_DATA*>::iterator iter(s_changedRooms.begin()); iter != s_changedRooms.end(); ++iter)
        ShowNew(**iter);

    // Update attunements and save
    UpdateAttunements();
    SaveWeave();
}

void Weave::Regenerate(AREA_DATA & area)
{
    // Build a list of rooms by frequency, cleaning up and tallying founts on the way
    // Do not put rooms with existing or just-removed founts on the list
    std::vector<ROOM_INDEX_DATA *> rooms;
    std::vector<ROOM_INDEX_DATA *> fountRooms;
    for (const VNUM_RANGE * range(area.vnums); range != NULL; range = range->next)
    {
        for (int i(range->min_vnum); i <= range->max_vnum; ++i)
        {
            // Get this room
            ROOM_INDEX_DATA * room(get_room_index(i));
            if (room == NULL)
                continue;

            // Check whether there is a fount here
            if (room->ley_group == NULL || !room->ley_group->HasFount())
                rooms.push_back(room);
            else
                fountRooms.push_back(room);
        }
    }

    // Calculate how many founts we want to have in the area, then clean up founts in excess of this
    unsigned int roomCount(rooms.size() + fountRooms.size());
    unsigned int fountCount(fountRooms.size());
    unsigned int newFountCount((roomCount * Fount::FountsPer100Rooms(area.fount_frequency)) / (number_range(80, 125)));
    while (fountCount > newFountCount && !fountRooms.empty())
    {
        // Pick a random room from the set
        unsigned int index(number_range(0, fountRooms.size() - 1));
        if (fountRooms[index]->fount_frequency != Fount::Always)
        {
            RemoveFount(*fountRooms[index]);
            --fountCount;
        }

        // Remove the room from the set regardless of whether the fount was cleared out
        fountRooms[index] = fountRooms[fountRooms.size() - 1];
        fountRooms.pop_back();
    }

    // Now test for more fount removal, not just that necessitated by an excess number
    for (size_t i(0); i < fountRooms.size(); ++i)
    {
        // Check the room type for frequency
        ROOM_INDEX_DATA * room(fountRooms[i]);
        Fount::Frequency freq(room->fount_frequency);
        if (freq == Fount::Default)
        {
            freq = room->area->fount_frequency;
            if (freq == Fount::Default)
                freq = Fount::Occasionally;
        }

        // Convert frequency into a numerical multiplier
        int oddsMultiplier(-1);
        switch (freq)
        {
            case Fount::Always: break;
            case Fount::Often: oddsMultiplier = 1; break;
            case Fount::Occasionally: oddsMultiplier = 2; break;
            case Fount::Rarely: oddsMultiplier = 4; break; 

            case Fount::Never: 
                bug("Somehow got a fount in a room marked Never (was the room recently edited?)", 0);
                oddsMultiplier = 1000;
                break;

            default:
                bug("Invalid frequency for room in fount cleanup", 0);
                break;
        }
                
        // Now calculate odds based on order power
        int chanceTenths(0);
        switch (room->ley_group->FountOrderPower())
        {
            case 2: chanceTenths = 2; break;
            case 1: chanceTenths = 4; break;
            case 0: chanceTenths = 8; break;
            case -1: chanceTenths = 16; break;
            case -2: chanceTenths = 32; break;
            default: bug("Invalid order power in fount cleanup calculation", 0); break;
        }

        // Now determine whether to actually remove the fount
        int randomValue(number_range(1, 1000));
        if (randomValue <= (chanceTenths * oddsMultiplier))
        {
            RemoveFount(*room);
            --fountCount;
            continue;
        }

        // Fount remains; check whether to rebuild the ley lines
        chanceTenths *= 16;
        if (randomValue <= chanceTenths)
            RerouteFount(*room);
    }

    // Now add in the always rooms
    unsigned int i(0);
    while (i < rooms.size())
    {
        if (rooms[i]->fount_frequency != Fount::Always)
        {
            ++i;
            continue;
        }

        // Found an always room, add the fount
        CreateFount(*rooms[i]);
        ++fountCount;
        rooms[i] = rooms[rooms.size() - 1];
        rooms.pop_back();
    }

    // Now add in any new founts
    for (i = fountCount; i < newFountCount && !rooms.empty(); ++i)
    {
        unsigned int loop(0);
        while (loop < 100)
        {
            // Choose a random room and test to see whether to pass it by
            unsigned int index(static_cast<unsigned int>(number_range(0, rooms.size() - 1)));
            if (number_percent() < static_cast<int>(Fount::SkipChance(rooms[index]->fount_frequency)))
            {
                // The loop check is just to avoid the theoretical (but unlikely) infinite loop
                ++loop;
                continue;
            }

            // Not skipping this one, create the fount and drop this room from the possibilities list
            CreateFount(*rooms[index]);
            rooms[index] = rooms[rooms.size() - 1];
            rooms.pop_back();
            break;
        }
    }
}

void Weave::RemoveFount(ROOM_INDEX_DATA & room)
{
    ClearLeyLines(room);
    room.ley_group->ClearFount(); 
}

void Weave::RerouteFount(ROOM_INDEX_DATA & room)
{
    ClearLeyLines(room);
    GenerateLeyLines(room);
}

void Weave::CreateFount(ROOM_INDEX_DATA & room)
{
    // Calculate fount powers
    int orderPower;
    int positivePower;
    do
    {
        orderPower = CalculateOrderPower(room);
        positivePower = CalculatePositivePower(room);
    } 
    while (orderPower == 0 && positivePower == 0);
    
    // Build the fount
    LeyInfo fount(&room, orderPower, positivePower);
    if (room.ley_group == NULL) room.ley_group = new LeyGroup(fount);
    else room.ley_group->SetFount(fount);

    // Generate the ley lines
    GenerateLeyLines(room);
}

int Weave::CalculatePower(int bias)
{
    int result((number_range(0, Fount::PowerMax) - number_range(0, Fount::PowerMax) + bias));
    if (result > Fount::PowerMax) return Fount::PowerMax;
    if (result < Fount::PowerMin) return Fount::PowerMin;
    return result;
}

int Weave::CalculateOrderPower(const ROOM_INDEX_DATA & room)
{
    // Check the room
    if (room.fount_order_power != 0)
        return room.fount_order_power;

    // Determine bias; this is where things like seasonal modifiers can go
    int bias(room.area->fount_order_bias);
    return CalculatePower(bias);
}

int Weave::CalculatePositivePower(const ROOM_INDEX_DATA & room)
{
    // Check the room
    if (room.fount_positive_power != 0)
        return room.fount_positive_power;

   // Determine bias; this is where things like seasonal modifiers go
   int bias(room.area->fount_positive_bias);
   return CalculatePower(bias);
}

void Weave::ClearLeyLines(ROOM_INDEX_DATA & room)
{
    // Iterate all rooms in the world; can't just track down the ley line because of areas with shifting exits
    s_changedRooms.insert(&room);
    for (unsigned int i(0); i < MAX_KEY_HASH; ++i)
    {
        for (ROOM_INDEX_DATA * roomIter(room_index_hash[i]); roomIter != NULL; roomIter = roomIter->next)
        {
            // Check whether this room has a ley line of matching id
            if (roomIter->ley_group == NULL)
                continue;

            unsigned int index(roomIter->ley_group->LineByID(&room));
            if (index != LeyGroup::NotFound)
            {
                roomIter->ley_group->Remove(index);
                s_changedRooms.insert(&room);
            }
        }
    }
}

void Weave::GenerateLeyLines(ROOM_INDEX_DATA & room)
{
    // Choose a random direction
    s_changedRooms.insert(&room);
    std::vector<Direction::Value> validDirections(Direction::ValidDirectionsFrom(room));
    Direction::Value direction(PopNextDirection(room, validDirections));
    if (direction == Direction::Max)
        return;

    // Proceed down that direction
    unsigned int leyLength((AverageLeyLength * number_range(50, 150)) / 100);
    ContinueLeyLine(room, direction, &room, 0, UMAX(leyLength, MinLeyLength), room.ley_group->FountOrderPower(), room.ley_group->FountPositivePower());
}

void Weave::ContinueLeyLine(ROOM_INDEX_DATA & room, Direction::Value direction, void * id, unsigned int length, unsigned int expectedLength, int orderPower, int positivePower)
{
    // Check whether to terminate here
    if (length >= expectedLength)
        return;

    // Determine whether to try to continue down this path; more likely if more ordered
    std::vector<Direction::Value> newDirections;
    if (Direction::Adjacent(room, direction) != NULL && number_percent() < (80 + (orderPower * 10)))
        newDirections.push_back(direction);
    else
    {
        // Not necessarily continuing down the same path (though it could still happen by chance)
        // Build a list of directions with valid rooms, ignoring the way back
        std::vector<Direction::Value> validDirections(Direction::ValidDirectionsFrom(room, Direction::ReverseOf(direction)));
        if (validDirections.empty())
            return;

        // Check how many branches to make
        unsigned int branches(1);
        while (branches <= validDirections.size() && number_percent() < 10 + (orderPower * 5))
            ++branches;
        
        // Produce the branches
        for (unsigned int i(0); i < branches; ++i)
        {
            // Get a random direction
            Direction::Value branchDirection(PopNextDirection(room, validDirections));
            if (branchDirection == Direction::Max)
                break;

            newDirections.push_back(branchDirection);
        }
    }

    // Add the ley info to each chosen room
    for (unsigned int i(0); i < newDirections.size(); ++i)
    {
        // TODO: should this LeyInfo vary at all?
        // Set up the ley group
        LeyInfo info(id, orderPower, positivePower);
        ROOM_INDEX_DATA & targetRoom(*Direction::Adjacent(room, newDirections[i]));
        if (targetRoom.ley_group == NULL)
            targetRoom.ley_group = new LeyGroup();
        
        // Add the ley info if not already present
        if (id != &targetRoom && targetRoom.ley_group->LineByID(id) == LeyGroup::NotFound)
            targetRoom.ley_group->Add(info);

        // Recursively generate with the new direction
        s_changedRooms.insert(&room);
        ContinueLeyLine(targetRoom, newDirections[i], id, length + 1, expectedLength, orderPower, positivePower);
    }
}

Direction::Value Weave::PopNextDirection(const ROOM_INDEX_DATA & room, std::vector<Direction::Value> & directions)
{
    // Sanity check
    if (directions.empty())
        return Direction::Max;

    // Loop for skipping
    for (unsigned int i(0); i < 10; ++i)
    {
        // Choose a random direction and check it for skipping
        unsigned int index(number_range(0, directions.size() - 1));
        Direction::Value result(directions[index]);
        if (number_percent() < static_cast<int>(Fount::SkipChance(Direction::Adjacent(room, result)->fount_frequency)))
            continue;

        // Direction chosen, pop it from the set
        directions[index] = directions[directions.size() - 1];
        directions.pop_back();
        return result;
    }

    // No valid result found
    return Direction::Max;
}
