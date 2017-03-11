#include "ShadeControl.h"
#include "RoomPath.h"
#include <sstream>
#include "NameMaps.h"

std::vector<const ROOM_INDEX_DATA *> ShadeControl::SpectralLanternRooms()
{
    // Get all the rooms with spectral lanterns
    std::vector<const ROOM_INDEX_DATA *> rooms;
    for (unsigned int i(0); i < MAX_KEY_HASH; ++i)
    {
        for (const ROOM_INDEX_DATA * room(room_index_hash[i]); room != NULL; room = room->next)
        {
            if (room_is_affected(const_cast<ROOM_INDEX_DATA*>(room), gsn_spectrallantern))
                rooms.push_back(room);
        }
    }

    return rooms;
}

void ShadeControl::UpdateShadePath(CHAR_DATA & ch, const std::vector<const ROOM_INDEX_DATA*> & rooms)
{
    // Find the nearest lantern room within 50 steps
    for (size_t i(0); i < rooms.size(); ++i)
    {
        // Find the path, but only count it if closer than the current path for the shade
        RoomPath path(*ch.in_room, *rooms[i], &ch, 50);
        if (!path.Exists() || (ch.path != NULL && ch.path->StepsRemaining() <= path.StepCount()))
            continue;

        // This path is closer, so assign it to the shade
        AssignNewPath(&ch, path);
    }
}

void ShadeControl::UpdateShadePathing()
{
    // Iterate the shades, looking for closest lanterns
    std::vector<const ROOM_INDEX_DATA *> rooms(SpectralLanternRooms());
    for (CHAR_DATA * ch(char_list); ch != NULL; ch = ch->next)
    {
        // Check for shades in real rooms
        if (!IS_NPC(ch) || !IS_SET(ch->nact, ACT_SHADE) || ch->in_room == NULL)
            continue;

        // Update this shade
        UpdateShadePath(*ch, rooms);
    }
}

bool ShadeControl::CheckShadeTimer(CHAR_DATA & ch)
{
    // Filter out any non-shades or shades not on a timer
    if (!IS_NPC((&ch)) || !IS_SET(ch.nact, ACT_SHADE) || ch.timer <= 0)
        return false;

    // Decrement the timer and check for vanishing
    --ch.timer;
    if (ch.timer > 0)
    {
        // Check for a new translucency stage
        if (ch.timer % 40 == 39)
        {
            DescribeShade(ch);
            ShowShadeMessage(ch, "$N shimmers briefly as $S form weakens, growing fainter.");
        }

        return false;
    }

    // Shade is vanishing
    ShowShadeMessage(ch, "Wisps of aether unravel from $N's form, and $E slowly dissipates into nothingness!");
    extract_char(&ch, true);
    return true;
}

bool ShadeControl::CheckShadeAttacked(CHAR_DATA & ch, CHAR_DATA & victim)
{
    // Filter out non-shades
    if (!IS_NPC((&victim)) || !IS_SET(victim.nact, ACT_SHADE))
        return false;

    // Found a shade; prevent attack
    act("$N is unaffected by your assault!", &ch, NULL, &victim, TO_CHAR);
    return true;
}

void ShadeControl::Update()
{
    // Get all spectral lanterns
    std::vector<const ROOM_INDEX_DATA *> lanterns(SpectralLanternRooms());

    // Iterate all areas in the world
    for (const AREA_DATA * area(area_first); area != NULL; area = area->next)
    {
        std::vector<ROOM_INDEX_DATA*> rooms;
        unsigned int shadeCount(CountShadesAndRooms(*area, rooms));
        UpdateArea(*area, shadeCount, rooms, lanterns);
    }
}

unsigned int ShadeControl::CountShadesAndRooms(const AREA_DATA & area, std::vector<ROOM_INDEX_DATA*> & rooms)
{
    // Iterate the rooms in this area
    unsigned int shadeCount(0);
    for (const VNUM_RANGE * range(area.vnums); range != NULL; range = range->next)
    {
        for (int i(range->min_vnum); i <= range->max_vnum; ++i)
        {
            // Get this room
            ROOM_INDEX_DATA * room(get_room_index(i));
            if (room == NULL)
                continue;

            // Iterate the people in the room
            rooms.push_back(room);
            for (const CHAR_DATA * ch(room->people); ch != NULL; ch = ch->next_in_room)
            {
                // Check for shadiness
                if (IS_NPC(ch) && IS_SET(ch->nact, ACT_SHADE))
                    ++shadeCount;
            }
        }
    }

    return shadeCount;
}

void ShadeControl::UpdateArea(const AREA_DATA & area, unsigned int shadeCount, std::vector<ROOM_INDEX_DATA*> & rooms, const std::vector<const ROOM_INDEX_DATA *> & lanterns)
{
    // Calculate how many shades this area caps at, then start randomly selecting rooms to possibly generate shades in
    unsigned int maxShades((Shades::CountPer100RoomsFor(area.shade_density) * rooms.size()) / 100);
    while (!rooms.empty() && shadeCount < maxShades)
    {
        // Update the room
        unsigned int index(number_range(0, rooms.size() - 1));
        if (UpdateRoom(*rooms[index], lanterns))
            ++shadeCount;

        // Remove the room from the list
        rooms[index] = rooms[rooms.size() - 1];
        rooms.pop_back();
    }
}

bool ShadeControl::UpdateRoom(ROOM_INDEX_DATA & room, const std::vector<const ROOM_INDEX_DATA*> & lanterns)
{
    // Determine density for the room
    Shades::Density density(room.shade_density);
    if (density == Shades::DefaultDensity)
        density = room.area->shade_density;

    // Spectral lantern heavily increases odds of generating a shade
    int bonusChance(0);
    if (density != Shades::Empty && room_is_affected(&room, gsn_spectrallantern))
        bonusChance += 50;

    // Use the density to determine whether to skip this room
    if (number_percent() > static_cast<int>(Shades::RoomChanceFor(density)) + bonusChance)
        return false;

    // We will be generating a shade; determine power for the room
    Shades::Power roomPower(room.shade_power);
    if (roomPower == Shades::DefaultPower)
    {
        roomPower = room.area->shade_power;
        if (roomPower == Shades::DefaultPower)
            roomPower = Shades::Average;
    }

    // Determine power; note that in int form, lowest power is a higher number than highest power
    Shades::Power power;
    while (true)
    {
        // Calculate a random number centered on 0 to build a normalish distribution
        int basePower(number_range(Shades::HighestPower, Shades::LowestPower * 2) - number_range(Shades::HighestPower, Shades::LowestPower * 2));

        // Add in the room power and bail out unless out of range
        power = static_cast<Shades::Power>(basePower + roomPower);
        if (power >= Shades::HighestPower && power <= Shades::LowestPower)
            break;
    }

    // Generate a shade; look up the index
    MOB_INDEX_DATA * mobIndex(get_mob_index(MOB_VNUM_SHADE));
    if (mobIndex == NULL)
    {
        bug("Failed to find shade index!", 0);
        return false;
    }

    // Make the shade
    CHAR_DATA * shade(create_mobile(mobIndex));
    if (shade == NULL)
    {
        bug("Failed to generate shade", 0);
        return false;
    }

    // Fill out the shade fields
    const Shades::ShadeGenInfo & info(Shades::InfoFor(power));
    shade->level = info.Level - 5 + number_range(0, 10);
    shade->damroll = (info.HitDamRoll * number_range(70, 130)) / 100;
    shade->hitroll = shade->damroll;
    shade->damage[0] = info.DamDiceCount;
    shade->damage[1] = (info.DamDiceSize * number_range(80, 120)) / 100;
    shade->damage[2] = info.DamDiceBonus;
    shade->max_hit = (info.HP * number_range(60, 140)) / 100;
    shade->max_mana = (info.Mana * number_range(60, 140)) / 100;
    shade->hit = shade->max_hit;
    shade->mana = shade->max_mana;
    shade->timer = number_range(70, 239);
    DescribeShade(*shade);
    
    // Add the shade to the room
    char_to_room(shade, &room);
    std::ostringstream mess;
    mess << "The local wisps of aether slowly begin to bind together, forming into $N!";
    ShowShadeMessage(*shade, mess.str().c_str());

    // Set up any pathing for this shade
    UpdateShadePath(*shade, lanterns);
    return true;
}

void ShadeControl::DescribeShade(CHAR_DATA & shade)
{
    // Initialize the fields
    std::ostringstream nameField;
    std::ostringstream shortField;
    std::ostringstream longField;

    // Start by checking the remaining time
    const char * article("a");
    const char * adjective("");
    switch (shade.timer / 40)
    {
        case 0: adjective = "diaphanous"; break;
        case 1: adjective = "wispy"; break;
        case 2: adjective = "faded"; break;
        case 3: adjective = "translucent"; break;
        case 4: article = "an"; adjective = "ethereal"; break;
        case 5: adjective = "nearly-tangible"; break;
        default:
            bug("Unexpected shade timer encountered in describeshade", 0);
            article = "an";
            adjective = "otherworldly";
            break;
    }

    nameField << adjective;
    shortField << article << " " << adjective;
    longField << article << " " << adjective;

    // Now determine which noun to use for the shade itself
    // This is random but based on the id of the shade so that it won't change
    // "shade" is favored
    const char * noun("shade");
    unsigned int next_seed(rand());
    srand(shade.id);
    switch (rand() % 6)
    {
        case 0: noun = "wraith"; break;
        case 1: noun = "spectre"; break;
        case 2: noun = "ghost"; break;
        case 3: noun = "phantom"; break;
        default: noun = "shade"; break;
    }
    srand(next_seed);

    nameField << " " << noun;
    shortField << " " << noun;
    longField << " " << noun << " hovers here, ";

    // Now check the alignment
    if (IS_GOOD((&shade)))
    {
        // Check level
        if (shade.level <= 35) longField << "lost in its own happy memories.";
        else if (shade.level <= 45) longField << "imbued with a soft silver glow.";
        else longField << "radiating an aura of holiness.";
    }
    else if (IS_EVIL((&shade)))
    {
        // Check level
        if (shade.level <= 35) longField << "trapped by its own dark memories.";
        else if (shade.level <= 45) longField << "surrounded by a liminal darkness.";
        else longField << "radiating a deathly chill.";
    }
    else
    {
        // Check level
        if (shade.level <= 35) longField << "lost in its memories.";
        else if (shade.level <= 45) longField << "drifting through its own memories.";
        else longField << "radiating pure emotion.";
    }

    // Path the long desc with a newline and uppercase initial letter
    longField << "\n";
    std::string longFieldStr(longField.str());
    longFieldStr[0] = UPPER(longFieldStr[0]);

    // Replace the fields
    free_string(shade.short_descr);
    free_string(shade.long_descr);
    setName(shade, nameField.str().c_str());
    shade.short_descr = str_dup(shortField.str().c_str());
    shade.long_descr = str_dup(longFieldStr.c_str());
}

void ShadeControl::ShowShadeMessage(const CHAR_DATA & shade, const char * message)
{
    // Sanity check
    if (shade.in_room == NULL)
        return;

    // Iterate the people in the room
    for (CHAR_DATA * ch(shade.in_room->people); ch != NULL; ch = ch->next_in_room)
    {
        // Check for shroudsight
        if (can_see(ch, const_cast<CHAR_DATA*>(&shade)))
            act(message, ch, NULL, &shade, TO_CHAR);
    }
}
