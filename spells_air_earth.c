#include "spells_air_earth.h"
#include "magic.h"
#include "Direction.h"
#include "spells_air.h"
#include "spells_water.h"
#include "spells_fire.h"
#include "RoomPath.h"
#include <sstream>
#include <set>

typedef std::set<ROOM_INDEX_DATA*> RoomSet;
static const int LodestoneFlags(EX_CLOSED|EX_WALLED|EX_ICEWALL);

static RoomSet find_next_pillars(unsigned int maxRadius, ROOM_INDEX_DATA & startRoom, const RoomSet & pillarRooms)
{
    RoomSet ignoreRooms;
    RoomSet rooms;
    RoomSet nextRooms;
    RoomSet result;
    rooms.insert(&startRoom);

    // Iterate up to the specified radius
    for (unsigned int i(0); i < maxRadius && !rooms.empty(); ++i)
    {
        // Process each current room
        while (!rooms.empty())
        {
            // Process the first room
            RoomSet::iterator iter(rooms.begin());
            ROOM_INDEX_DATA * room(*iter);
            rooms.erase(iter);
            ignoreRooms.insert(room);

            // Examine all the connections
            for (unsigned int dir(0); dir < Direction::Max; ++dir)
            {
                // Look for a next room
                ROOM_INDEX_DATA * nextRoom(Direction::Adjacent(*room, static_cast<Direction::Value>(dir)));
                if (nextRoom == NULL || pillarRooms.find(nextRoom) != pillarRooms.end() || ignoreRooms.find(nextRoom) != ignoreRooms.end() || rooms.find(nextRoom) != rooms.end())
                    continue;

                // Check whether this room has a pillar
                if (room_is_affected(nextRoom, gsn_pillarofsparks)) result.insert(nextRoom);
                nextRooms.insert(nextRoom);
            }
        }

        // Swap the room sets to prepare for the next iteration
        std::swap(rooms, nextRooms);
    }

    return result;
}

static void deal_pillar_damage(CHAR_DATA & ch, int level, ROOM_INDEX_DATA & room)
{
    // Blast everyone in the room but the caster
    CHAR_DATA * victim_next;
    for (CHAR_DATA * victim(room.people); victim != NULL; victim = victim_next)
    {
        // Check for whether the victim should be hit
        victim_next = victim->next_in_room;
        if ((ch.in_room == victim->in_room && is_same_group(&ch, victim)) || is_safe_spell(&ch, victim, true))
            continue;

        // Deal damage
        int dam(number_range(level, level * 3));
        if (saves_spell(level, &ch, victim, DAM_LIGHTNING))
            dam /= 2;

        if (ch.in_room == victim->in_room)
            damage(&ch, victim, dam, gsn_pillarofsparks, DAM_LIGHTNING, true);
        else
        {
            if (!IS_NPC(victim) || victim->hit >= (victim->max_hit * 9 / 10))
                sourcelessDamage(victim, "the arc of lightning", dam, gsn_pillarofsparks, DAM_LIGHTNING);
            else
            {
                act("You avoid the blast!", victim, NULL, NULL, TO_CHAR);
                act("$n avoids the blast!", victim, NULL, NULL, TO_ROOM);
            }
        }
    }
}

static void handle_next_pillars(CHAR_DATA & ch, int level, ROOM_INDEX_DATA & room, RoomSet & pillarRooms)
{
    // Get the next batch of pillars
    RoomSet nextPillars(find_next_pillars(level / 6, room, pillarRooms));
    if (nextPillars.empty())
        return;

    // Iterate these pillars
    RoomSet zappedRooms;
    for (RoomSet::const_iterator iter(nextPillars.begin()); iter != nextPillars.end(); ++iter)
    {
        // Find a path to the pillar
        pillarRooms.insert(*iter);
        RoomPath path(room, **iter, NULL, RoomPath::Infinite, EX_WALLED|EX_CLOSED);
        if (!path.Exists() || path.StepCount() == 0)
            continue;

        // Echo the bolt leaving
        std::ostringstream echo;
        echo << "An arc of lightning leaps from the pillar, flashing off ";
        echo << Direction::DirectionalNameFor(static_cast<Direction::Value>(path.StepDirection(0))) << '!';
        act(echo.str().c_str(), room.people, NULL, NULL, TO_ALL);

        // Walk the path to the pillar
        ROOM_INDEX_DATA * prevRoom(&room);
        for (size_t i(0); i + 1 < path.StepCount(); ++i)
        {
            // Find the next room in the path
            Direction::Value direction(static_cast<Direction::Value>(path.StepDirection(i)));
            ROOM_INDEX_DATA * nextRoom(Direction::Adjacent(*prevRoom, direction));
            if (nextRoom == NULL)
            {
                bug("Null nextRoom in handle_next_pillars despite having found a path", 0);
                break;
            }

            // Echoes
            std::ostringstream mess;
            mess << "A bolt of crackling lightning arcs in " << Direction::SourceNameFor(Direction::ReverseOf(direction));
            mess << ", flashing " << Direction::DirectionalNameFor(static_cast<Direction::Value>(path.StepDirection(i + 1)));
            mess << "!";
            act(mess.str().c_str(), nextRoom->people, NULL, NULL, TO_ALL);
            
            // Deal damage and advance
            if (zappedRooms.find(nextRoom) == zappedRooms.end())
            {
                zappedRooms.insert(nextRoom);
                deal_pillar_damage(ch, level, *nextRoom);
            }
            prevRoom = nextRoom;
        }

        // Zap the pillar room itself now
        std::ostringstream mess;
        mess << "A bolt of crackling lightning arcs in ";
        mess << Direction::SourceNameFor(Direction::ReverseOf(static_cast<Direction::Value>(path.StepDirection(path.StepCount() - 1))));
        mess << ", striking the metal pillar in a burst of sparks and scattering about!";
        act(mess.str().c_str(), (*iter)->people, NULL, NULL, TO_ALL);

        if (zappedRooms.find(*iter) == zappedRooms.end())
        {
            zappedRooms.insert(*iter);
            deal_pillar_damage(ch, level, **iter);
        }
    }

    // Now recursively zap the next pillar set
    for (RoomSet::const_iterator iter(nextPillars.begin()); iter != nextPillars.end(); ++iter)
        handle_next_pillars(ch, level, **iter, pillarRooms);
}

void handle_pillar_attacked(CHAR_DATA & ch, int level)
{
    // Initial echoes and damage
    act("You send a bolt of lightning leaping from your fingers to strike the metal pillar here!", &ch, NULL, NULL, TO_CHAR);
    act("A bolt of lightning leaps from $n's fingers, striking the metal pillar here!", &ch, NULL, NULL, TO_ROOM);
    act("As soon as the bolt hits, it shatters into arcs of crackling energy, which leap about this place!", &ch, NULL, NULL, TO_ALL);
    deal_pillar_damage(ch, level, *ch.in_room);

    // Prepare to walk the pillar chain
    RoomSet pillarRooms;
    pillarRooms.insert(ch.in_room);
    handle_next_pillars(ch, level, *ch.in_room, pillarRooms);

    // Reduce pillar durations
    for (RoomSet::iterator iter(pillarRooms.begin()); iter != pillarRooms.end(); ++iter)
    {
        // Check for a pillar effect
        AFFECT_DATA * paf(get_room_affect(*iter, gsn_pillarofsparks));
        if (paf == NULL)
            continue;

        // Reduce the pillar's duration
        paf->duration -= number_range(0, 6);
        if (paf->duration <= 0)
        {
            act("The metal pillar crumbles from the blast!", (*iter)->people, NULL, NULL, TO_ALL);
            room_affect_strip(*iter, gsn_pillarofsparks);
        }
    }
}

bool spell_pillarofsparks(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    // Room checks
    if (ch->in_room == NULL || !ON_GROUND(ch))
    {
        send_to_char("You cannot conjure a pillar of sparks in such a place.\n", ch);
        return false;
    }

    // Check for cooldown
    if (is_affected(ch, sn))
    {
        send_to_char("You are not ready to summon another pillar of sparks yet.\n", ch);
        return false;
    }

    // Check for pillar already present
    if (room_is_affected(ch->in_room, sn))
    {
        send_to_char("There is already a pillar here!\n", ch);
        return false;
    }

    // Echoes
    act("You bellow a word of power, and a solid metal pillar erupts from the ground, crackling with energy!", ch, NULL, NULL, TO_CHAR);
    act("$n bellows a word of power, and a solid metal pillar erupts from the ground, crackling with energy!", ch, NULL, NULL, TO_ROOM);

    // Apply cooldown
    int skill(get_skill(ch, sn));
    AFFECT_DATA af = {0};
    af.where    = TO_AFFECTS;
    af.type     = sn;
    af.level    = level;
    af.duration = 10 - (UMAX(0, skill - 70) / 6);
    affect_to_char(ch, &af);

    // Apply actual effect
    af.where    = TO_ROOM_AFF;
    af.duration = (level * 2);
    affect_to_room(ch->in_room, &af);

    return true;
}

static bool is_magnet_candidate(OBJ_DATA & obj)
{
    if (!material_table[obj.material].metal) return false;
    if (IS_OBJ_STAT(&obj, ITEM_WIZI)) return false;
    if (IS_OBJ_STAT(&obj, ITEM_NOLONG)) return false;
    if (IS_OBJ_STAT(&obj, ITEM_INVENTORY)) return false;
    if (IS_OBJ_STAT(&obj, ITEM_NOREMOVE)) return false;
    if (IS_OBJ_STAT(&obj, ITEM_VIS_DEATH)) return false;
    if (IS_OBJ_STAT(&obj, ITEM_AFFINITY)) return false;
    if (IS_OBJ_STAT(&obj, ITEM_HIDDEN)) return false;
    if (IS_OBJ_STAT(&obj, ITEM_STASHED)) return false;
    if (!CAN_WEAR(&obj, ITEM_TAKE)) return false;
    if (obj.weight > 50) return false;
    return true;
}

static void update_lodestone_magnet(OBJ_DATA & obj)
{
    // Sanity-check
    if (obj.in_room == NULL)
        return;

    // Check for light metal objects on the ground
    std::vector<OBJ_DATA*> items;
    for (OBJ_DATA * item(obj.in_room->contents); item != NULL; item = item->next_content)
    {
        if (is_magnet_candidate(*item) && number_bits(1) == 0)
        {
            act("$p flies up suddenly, clinging fast to the floating stone!", obj.in_room->people, item, NULL, TO_ALL);
            items.push_back(item);
        }
    }

    // Check for light metal objects in inventories
    std::vector<OBJ_DATA*> invItems;
    for (CHAR_DATA * victim(obj.in_room->people); victim != NULL; victim = victim->next_in_room)
    {
        if (number_percent() <= get_skill(victim, gsn_stonecraft))
            continue;

        for (OBJ_DATA * item(victim->carrying); item != NULL; item = item->next_content)
        {
            if (!item->worn_on && is_magnet_candidate(*item) && number_bits(4) == 0)
            {
                act("$p flies up suddenly away from you, clinging fast to the floating stone!", victim, item, NULL, TO_CHAR);
                act("$p flies up suddenly away from $n, clinging fast to the floating stone!", victim, item, NULL, TO_ROOM);
                invItems.push_back(item);
            }
        }

        // Check for coins
        for (int coinType(0); coinType < MAX_COIN; ++coinType)
        {
            if (victim->coins[coinType] > 0 && number_bits(6) == 0)
            {
                int stealAmount(number_range(1, UMIN(3, victim->coins[coinType])));
                coins_from_char(victim, stealAmount, coinType);
                OBJ_DATA * item(create_money(stealAmount, coinType));
                act("$p flies up and away from you, clinging fast to the floating stone!", victim, item, NULL, TO_CHAR);
                act("$p flies up and away from $n, clinging fast to the floating stone!", victim, item, NULL, TO_ROOM);
                obj_to_obj(item, &obj);
            }
        }
    }

    // Now actually move the objects
    for (size_t i(0); i < items.size(); ++i)
    {
        obj_from_room(items[i]);
        obj_to_obj(items[i], &obj);
    }

    for (size_t i(0); i < invItems.size(); ++i)
    {
        obj_from_char(invItems[i]);
        obj_to_obj(invItems[i], &obj);
    }
}

/// A false result means failed to move
static bool update_lodestone_move(OBJ_DATA & obj, Direction::Value direction)
{
    // Get he room
    ROOM_INDEX_DATA * room(Direction::Adjacent(*obj.in_room, direction, LodestoneFlags));
    if (room == NULL)
        return false;

    // Echo to the current room
    std::ostringstream mess;
    mess << "Wobbling unsteadily, $p spins slowly off " << Direction::DirectionalNameFor(direction) << ".";
    act(mess.str().c_str(), obj.in_room->people, &obj, NULL, TO_ALL);

    // Move the lodestone
    obj_from_room(&obj);
    obj_to_room(&obj, room);

    // Echo to the new room
    mess.str("");
    mess << "$p glides in " << Direction::SourceNameFor(Direction::ReverseOf(direction)) << ", spinning slowly in the air.";
    act(mess.str().c_str(), obj.in_room->people, &obj, NULL, TO_ALL); 
    update_lodestone_magnet(obj);
    return true;
}

// A true result means the effect can be removed
static bool update_lodestone_helper(OBJ_DATA & obj, AFFECT_DATA & aff)
{
    // Sanity-check
    if (obj.in_room == NULL)
        return true;

    // Check whether still voyaging
    if (aff.location == APPLY_NONE)
    {
        // Still voyaging, check whether to start returning
        if (number_bits(10) != 0)
        {
            // Not returning yet; try to continue down the same direction
            Direction::Value direction(static_cast<Direction::Value>(aff.level));
            if (direction == Direction::Max || number_bits(4) == 0 
            || Direction::Adjacent(*obj.in_room, direction, LodestoneFlags) == NULL)
            {
                // Choose a random direction
                std::vector<Direction::Value> directions(Direction::ValidDirectionsFrom(*obj.in_room, Direction::Max, LodestoneFlags));
                if (!directions.empty())
                    direction = directions[number_range(0, directions.size() - 1)];
            }

            // Try to move in that direction           
            if (direction != Direction::Max) 
            {
                aff.level = direction;
                update_lodestone_move(obj, direction);
            }

            return false;
        }
        
        // Start returning
        aff.location = APPLY_HIDE;
    }

    // The object is heading back now, so look for the target
    CHAR_DATA * target(get_char_by_id_any(aff.modifier));
    if (target == NULL || target->in_room == NULL || target->in_room == obj.in_room)
        return true;

    // Find a path to the target
    RoomPath path(*obj.in_room, *target->in_room, NULL, RoomPath::Infinite, LodestoneFlags);
    if (!path.Exists() || path.StepCount() == 0)
        return true;

    // Move the lodestone
    if (!update_lodestone_move(obj, static_cast<Direction::Value>(path.StepDirection(0))))
        return true;

    // If the lodestone moved into the target's room, dump it
    if (obj.in_room == target->in_room)
        return true;

    return false;
}

static bool update_lodestone(OBJ_DATA & obj, AFFECT_DATA & aff)
{
    if (!update_lodestone_helper(obj, aff))
        return false;
    
    // Dump the lodestone
    if (obj.in_room != NULL)
    {
        act("$p wobbles uncertainly in the air, then suddenly crashes to the ground!", obj.in_room->people, &obj, NULL, TO_ALL);
        for (OBJ_DATA * item(obj.contains); item != NULL; item = obj.contains)
        {
            act("$p falls off, no longer stuck fast to the stone!", obj.in_room->people, item, NULL, TO_ALL);
            obj_from_obj(item);
            obj_to_room(item, obj.in_room);
        }
    }

    affect_remove_obj(&obj, &aff);
    return true;
}

static std::vector<OBJ_DATA *> s_lodestones;

void lodestone_update()
{
    size_t i(0);
    while (i < s_lodestones.size())
    {
        // Handle the next lodestone
        OBJ_DATA * obj(s_lodestones[i]);
        if (IS_VALID(obj))
        {
            AFFECT_DATA * paf(get_obj_affect(obj, gsn_dispatchlodestone));
            if (paf != NULL && !update_lodestone(*obj, *paf))
            {
                // Lodestone is valid and still roving, move on to the next one
                ++i;
                continue;
            }
        }

        // Remove the lodestone from the list
        s_lodestones[i] = s_lodestones[s_lodestones.size() - 1];
        s_lodestones.pop_back();
    }
}

bool spell_dispatchlodestone(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    // Check for cooldown
    if (is_affected(ch, sn))
    {
        send_to_char("You are not yet ready to charge another lodestone.\n", ch);
        return false;
    }

    // Check for room
    if (ch->in_room == NULL || ch->in_room->sector_type == SECT_UNDERWATER)
    {
        send_to_char("You cannot dispatch a lodestone from this place!\n", ch);
        return false;
    }

    // Check for a stone object
    OBJ_DATA * obj(static_cast<OBJ_DATA*>(vo));
    if (!material_table[obj->material].stone)
    {
        send_to_char("Only items made of stone may be charged as a lodestone.\n", ch);
        return false;
    }

    // Make sure there is nothing in it first
    if (obj->contains != NULL)
    {
        send_to_char("You should probably empty it first.\n", ch);
        return false;
    }

    // Apply a cooldown
    AFFECT_DATA af = {0};
    int skill(get_skill(ch, sn));
    af.where    = TO_AFFECTS;
    af.type     = sn;
    af.level    = level;
    af.duration = 120 - UMAX(0, skill - 70);
    affect_to_char(ch, &af);

    // Prepare the object
    af.where    = TO_OBJECT;
    af.duration = -1;
    af.level    = Direction::Max;
    af.location = APPLY_NONE;
    af.modifier = ch->id;
    affect_to_obj(obj, &af);

    // Echoes
    act("You place a hand gently on $p, willing energy into it.", ch, obj, NULL, TO_CHAR);
    act("$n places a hand gently on $p, and it begins to hum with power!", ch, obj, NULL, TO_ROOM);
    act("$p rises into the air, spinning slowly!", ch, obj, NULL, TO_ALL);
    obj_from_char(obj);
    obj_to_room(obj, ch->in_room);

    s_lodestones.push_back(obj);
    update_lodestone_magnet(*obj);
    return true;
}

bool spell_sandstorm(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    // Check for cooldown
    if (is_affected(ch, sn))
    {
        send_to_char("You are not yet ready to summon another sandstorm.\n", ch);
        return false;
    }

    // Check for room
    if (ch->in_room == NULL || !ON_GROUND(ch))
    {
        send_to_char("You cannot reach both sand and wind from this place.\n", ch);
        return false;
    }

    // Check for area already affected
    if (area_is_affected(ch->in_room->area, sn))
    {
        send_to_char("There is already a sandstorm raging about this area!\n", ch);
        return false;
    }

    // Prepare to cast the spell
    send_to_char("You reach for earth and air, binding them together and urging them into a frenzy!\n", ch);

    AFFECT_DATA af = {0};
    af.where    = TO_ROOM;
    af.level    = level;
    af.type     = sn;
    af.duration = (level / 17) + number_range(1, 3);

    // Apply effect to all connected rooms up to the max
    std::set<ROOM_INDEX_DATA*> roomsProcessed;
    std::set<ROOM_INDEX_DATA*> currRooms;
    currRooms.insert(ch->in_room);

    unsigned int maxRooms(dice(4, level / 2));
    unsigned int count(0);
    while (!currRooms.empty() && count < maxRooms)
    {
        // Process the first room in currRooms
        std::set<ROOM_INDEX_DATA*>::iterator iter(currRooms.begin());
        ROOM_INDEX_DATA * room(*iter);
        currRooms.erase(iter);
        roomsProcessed.insert(room);

        // Potentially apply the effect to this room
        if (!is_water_room(*room) && room->sector_type != SECT_AIR)
        {
            ++count;
            affect_to_room(room, &af);
            act("A stinging torrent of wind and sand begins to swirl here!", room->people, NULL, NULL, TO_ALL);
        }

        // Grab all the links
        for (unsigned int i(0); i < Direction::Max; ++i)
        {
            ROOM_INDEX_DATA * nextRoom(Direction::Adjacent(*room, static_cast<Direction::Value>(i), EX_CLOSED));
            if (nextRoom != NULL && roomsProcessed.find(nextRoom) == roomsProcessed.end())
                currRooms.insert(nextRoom);
        }
    }

    // Apply a cooldown
    af.where    = TO_AFFECTS;
    af.duration = 60 - UMAX(0, (get_skill(ch, sn) - 70) / 3);
    affect_to_char(ch, &af);
    return true;
}

bool spell_channelwind(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    // Check for cooldown
    if (is_affected(ch, sn))
    {
        send_to_char("You are not prepared to form another windtunnel.\n", ch);
        return false;
    }

    // Check the room type
    if (ch->in_room == NULL || !ON_GROUND(ch))
    {
        send_to_char("There is not enough earth here to shape into a windtunnel.\n", ch);
        return false;
    }

    // Check for existing tunnels
    if (room_is_affected(ch->in_room, sn))
    {
        send_to_char("There is already a windtunnel here.\n", ch);
        return false;
    }

    // Figure out the direction
    if (target_name[0] == '\0')
    {
        send_to_char("In which direction did you wish to channel the wind?\n", ch);
        return false;
    }

    Direction::Value direction(Direction::ValueFor(target_name));
    if (direction == Direction::Max || Direction::Adjacent(*ch->in_room, direction, EX_CLOSED|EX_WALLED|EX_ICEWALL) == NULL)
    {
        send_to_char("That's not a valid direction for the tunnel.\n", ch);
        return false;
    }

    // Set up the tunnel effect
    AFFECT_DATA af = {0};
    af.where    = TO_ROOM;
    af.level    = level;
    af.type     = sn;
    af.modifier = direction;
    af.duration = (level / 17) + number_range(1, 4);

    // Echoes
    std::ostringstream mess;
    mess << "A fierce wind begins to blow " << Direction::DirectionalNameFor(direction) << " down the newly-formed channel, gathering strength as it rushes along!";
    act("You raise your hands with palms facing upwards, and two thin walls of stone rise up in parallel before you.", ch, NULL, NULL, TO_CHAR);
    act("$n raises $s hands with palms facing upwards, and two thin walls of stone rise up in parallel before $m.", ch, NULL, NULL, TO_ROOM);
    act(mess.str().c_str(), ch, NULL, NULL, TO_ALL);
    affect_to_room(ch->in_room, &af);

    mess.str("");
    mess << "A fierce wind suddenly rushes in " << Direction::SourceNameFor(Direction::ReverseOf(direction)) << "!";

    // Apply the effect to each room in turn
    unsigned int maxRooms(level / 10);
    unsigned int count(1);
    for (ROOM_INDEX_DATA * room(Direction::Adjacent(*ch->in_room, direction, EX_CLOSED|EX_WALLED|EX_ICEWALL)); room != NULL && count < maxRooms; room = Direction::Adjacent(*room, direction, EX_CLOSED|EX_WALLED|EX_ICEWALL))
    {
        ++count;
        affect_to_room(room, &af);
        act(mess.str().c_str(), room->people, NULL, NULL, TO_ALL);
    }

    // Apply a cooldown
    af.where    = TO_AFFECTS;
    af.duration = 22 - UMAX(0, (get_skill(ch, sn) - 70) / 5);
    affect_to_char(ch, &af);
    return true;
}

void handle_chargestone_destruction(CHAR_DATA * ch, OBJ_DATA * obj)
{
    // Check for chargestone
    AFFECT_DATA * stoneAff(get_obj_affect(obj, gsn_chargestone));
    if (stoneAff == NULL)
        return;

    // This is a chargestone, prepare the amount
    int amount(2 + (stoneAff->modifier / 6));
    int amountHP(2 + (stoneAff->modifier / 2));

    // Modify for shockcraft
    if (number_percent() <= get_skill(ch, gsn_shockcraft))
        check_improve(ch, NULL, gsn_shockcraft, true, 6);
    else
    {
        check_improve(ch, NULL, gsn_shockcraft, false, 6);
        amount = UMIN(amount / 2, 15);
        amountHP = UMIN(amountHP / 2, 50);
    }

    // Echoes
    act("A burst of crackling energy leaps from $p into $n!", ch, obj, NULL, TO_ROOM);
    act("Crackling energy infused with earthen magic flows from $p into you, reinforcing your very being!", ch, obj, NULL, TO_CHAR);
    
    // Check for effect already on user
    AFFECT_DATA * paf(get_affect(ch, gsn_chargestone));
    if (paf != NULL)
        affect_strip(ch, gsn_chargestone);
    
    // Apply the effect
    AFFECT_DATA af = {0};
    af.where    = TO_AFFECTS;
    af.level    = stoneAff->level;
    af.type     = gsn_chargestone;
    af.duration = stoneAff->level;
    af.location = APPLY_RESIST_WEAPON;
    af.modifier = amount;
    affect_to_char(ch, &af);

    af.location = APPLY_HIT;
    af.modifier = amountHP;
    affect_to_char(ch, &af);
    ch->hit = UMIN(ch->hit + af.modifier, ch->max_hit);
}

bool spell_chargestone(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    // Check for opal
    OBJ_DATA * obj(static_cast<OBJ_DATA*>(vo));
    if (obj->material != material_lookup("opal"))
    {
        send_to_char("Only items made of opal could properly store those magics.\n", ch);
        return false;
    }

    // Check for charge
    AFFECT_DATA * charged(get_charged_effect(ch));
    if (charged == NULL || charged->modifier < 25)
    {
        act("You have absorbed insufficient charge to infuse $p.", ch, obj, NULL, TO_CHAR);
        return false;
    }

    // Check for effect already present
    if (obj_is_affected(obj, sn))
    {
        act("$p is already a chargestone.", ch, obj, NULL, TO_CHAR);
        return false;
    }

    // Echoes
    act("You draw the stored charge from yourself, channeling it into $p!", ch, obj, NULL, TO_CHAR);
    act("$p sparks and crackles with energy, quickly quieting into a steady hum.", ch, obj, NULL, TO_ROOM);

    // Apply the effect, removing it from the caster
    AFFECT_DATA af = {0};
    af.where     = TO_OBJECT;
    af.level     = level;
    af.type      = sn;
    af.duration  = number_range(level * 3, level * 5);
    af.modifier  = charged->modifier - 25;
    af.bitvector = ITEM_HUM;
    affect_to_obj(obj, &af);
    affect_remove(ch, charged);
    return true;
}

bool spell_hoveringshield(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    // Check for shield
    OBJ_DATA * obj(static_cast<OBJ_DATA*>(vo));
    if (!IS_SET(obj->wear_flags, ITEM_WEAR_SHIELD))
    {
        send_to_char("That incantation only works on shields.\n", ch);
        return false;
    }

    // Check for already floating
    if (IS_SET(obj->wear_flags, ITEM_WEAR_FLOAT))
    {
        act("$p can already float on its own.", ch, obj, NULL, TO_CHAR);
        return false;
    }

    // Make sure the slot is empty
    if (get_eq_char(ch, WEAR_FLOAT) != NULL)
    {
        send_to_char("You already have something floating about you.\n", ch);
        return false;
    }

    // Make the item float
    AFFECT_DATA af = {0};
    af.where    = TO_OBJECT;
    af.type     = sn;
    af.level    = level;
    af.duration = number_range(5, 15);
    affect_to_obj(obj, &af);
    SET_BIT(obj->wear_flags, ITEM_WEAR_FLOAT);
    REMOVE_BIT(obj->wear_flags, ITEM_WEAR_SHIELD);

    // Echoes
    act("As you finish the incantation, $p rises into the air, circling you lazily.", ch, obj, NULL, TO_CHAR);
    act("$p suddenly rises into the air, circling lazily about $n.", ch, obj, NULL, TO_ROOM);
    equip_char(ch, obj, WORN_FLOAT);
    oprog_wear_trigger(obj);
    return true;
}
